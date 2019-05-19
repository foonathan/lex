// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_TOKENIZER_HPP_INCLUDED
#define FOONATHAN_LEX_TOKENIZER_HPP_INCLUDED

#include <foonathan/lex/detail/trie.hpp>
#include <foonathan/lex/identifier_token.hpp>
#include <foonathan/lex/literal_token.hpp>
#include <foonathan/lex/rule_token.hpp>
#include <foonathan/lex/token.hpp>
#include <foonathan/lex/whitespace_token.hpp>

namespace foonathan
{
namespace lex
{
    namespace detail
    {
        //=== literal trie building ===//
        template <class TokenSpec>
        struct literal_trie_impl
        {
            template <class... LiteralTokens>
            struct fn // empty
            {
                using type = typename trie<TokenSpec>::empty;
            };
            template <class Head, class... Tail>
            struct fn<Head, Tail...> // non-empty
            {
                using base = typename fn<Tail...>::type;
                using type = typename trie<TokenSpec>::template insert_literal<
                    base, token_kind<TokenSpec>(Head{}).get(), literal_token_type<Head>>;
            };
        };

        template <class TokenSpec, class LiteralTokens>
        using literal_trie =
            typename mp::mp_apply_q<literal_trie_impl<TokenSpec>, LiteralTokens>::type;

        //=== rule insertion ===//
        template <class TokenSpec, class Trie>
        struct rule_trie_impl
        {
            template <class... Rules>
            struct fn // empty
            {
                using type = Trie;
            };
            template <class Head, class... Tail>
            struct fn<Head, Tail...> // non-empty
            {
                using base = typename fn<Tail...>::type;
                using type = typename base::template insert_rule<Head>;
            };
        };

        template <class TokenSpec, class Trie, class RuleTokens>
        using rule_trie =
            typename mp::mp_apply_q<rule_trie_impl<TokenSpec, Trie>, RuleTokens>::type;

        //=== keyword trie ===//
        template <class TokenSpec, class Identifier, class... Keywords>
        struct keyword_identifier_matcher
        {
            using keyword_trie = literal_trie<TokenSpec, mp::mp_list<Keywords...>>;

            static constexpr bool is_conflicting_literal(token_kind<TokenSpec> kind) noexcept
            {
                return Identifier::is_conflicting_literal(kind);
            }

            static constexpr match_result<TokenSpec> try_match(const char* str,
                                                               const char* end) noexcept
            {
                auto identifier = Identifier::try_match(str, end);
                if (!identifier.is_success())
                    // not an identifier, so can't be a keyword
                    return identifier;

                // try to match a keyword in the identifier
                auto identifier_begin = str;
                auto identifier_end   = str + identifier.bump;
                auto keyword          = keyword_trie::try_match(identifier_begin, identifier_end);
                if (keyword.is_success() && keyword.bump == identifier.bump)
                    // we've matched a keyword and it isn't a prefix but the whole string
                    return keyword;
                else
                    // didn't match keyword or it was only a prefix
                    return identifier;
            }
        };

        template <class TokenSpec, class KeywordList>
        struct keyword_trie_impl
        {
            // create a matcher for that identifier and some keyword list
            template <class Identifier>
            struct matcher
            {
                template <class... Keywords>
                using fn = keyword_identifier_matcher<TokenSpec, Identifier, Keywords...>;
            };

            template <class... Identifiers>
            // create a list of rules, one for each identifier (i.e. 0 or 1 in practice)
            using fn = mp::mp_list<mp::mp_apply_q<matcher<Identifiers>, KeywordList>...>;
        };

        template <class TokenSpec, class Trie, class Identifiers, class Keywords>
        using keyword_trie
            = rule_trie<TokenSpec, Trie,
                        // insert the list of rules
                        mp::mp_apply_q<keyword_trie_impl<TokenSpec, Keywords>, Identifiers>>;

        //=== token_spec_trie ===//
        template <class TokenSpec>
        struct token_spec_trie_impl
        {
            using list = typename TokenSpec::list;

            // split the tokens
            using identifiers = mp::mp_copy_if<list, is_identifier_token>;
            using keywords    = mp::mp_copy_if<list, is_keyword_token>;
            using rule_tokens = mp::mp_copy_if<list, is_non_identifier_rule_token>;
            using literals    = mp::mp_copy_if<list, is_non_keyword_literal_token>;

            // verify identifier conditions
            static_assert(mp::mp_size<identifiers>::value <= 1,
                          "at most one identifier is allowed");
            static_assert(mp::mp_empty<keywords>::value || mp::mp_size<identifiers>::value > 0,
                          "keywords require an identifier");

            // start with the literal trie
            using trie0 = literal_trie<TokenSpec, literals>;
            // insert all rule tokens
            using trie1 = rule_trie<TokenSpec, trie0, rule_tokens>;
            // insert the keyword identifier rule
            using trie2 = keyword_trie<TokenSpec, trie1, identifiers, keywords>;
        };

        template <class TokenSpec>
        using token_spec_trie = typename token_spec_trie_impl<TokenSpec>::trie2;
    } // namespace detail

    template <class TokenSpec>
    class tokenizer
    {
        using trie = detail::token_spec_trie<TokenSpec>;
        static_assert(detail::mp::mp_all_of<typename TokenSpec::list, is_token>::value,
                      "invalid types in token specifications");

    public:
        //=== constructors ===//
        explicit constexpr tokenizer(const char* ptr, std::size_t size) noexcept
        : tokenizer(ptr, ptr + size)
        {}

        explicit constexpr tokenizer(const char* begin, const char* end)
        : begin_(begin), ptr_(begin), end_(end), last_result_(match_result<TokenSpec>::unmatched())
        {
            bump();
        }

        template <std::size_t N>
        explicit constexpr tokenizer(const char (&array)[N]) : tokenizer(array, array + N - 1)
        {}

        //=== tokenizer functions ===//
        constexpr token<TokenSpec> peek() const noexcept
        {
            return token<TokenSpec>(last_result_.kind, ptr_, last_result_.bump);
        }

        constexpr bool is_done() const noexcept
        {
            FOONATHAN_LEX_ASSERT(last_result_.bump != 0 || peek().is(eof_token{}));
            return last_result_.bump == 0;
        }

        constexpr token<TokenSpec> get() noexcept
        {
            auto result = peek();
            bump();
            return result;
        }

        constexpr void bump() noexcept
        {
            reset(ptr_ + last_result_.bump);
        }

        constexpr void reset(const char* position) noexcept
        {
            reset_impl(position);

            using any_whitespace
                = detail::mp::mp_any_of<typename TokenSpec::list, is_whitespace_token>;
            skip_whitespace(any_whitespace{});
        }

        //=== getters ===//
        constexpr const char* begin_ptr() const noexcept
        {
            return begin_;
        }

        constexpr const char* current_ptr() const noexcept
        {
            FOONATHAN_LEX_ASSERT(peek().spelling().data() == ptr_);
            return ptr_;
        }

        constexpr const char* end_ptr() const noexcept
        {
            return end_;
        }

    private:
        constexpr void reset_impl(const char* position) noexcept
        {
            FOONATHAN_LEX_PRECONDITION(begin_ <= position && position <= end_,
                                       "position out of range");
            ptr_         = position;
            last_result_ = trie::try_match(ptr_, end_);
        }

        constexpr void skip_whitespace(std::true_type)
        {
            while (last_result_.kind.template is_category<is_whitespace_token>())
                reset_impl(ptr_ + last_result_.bump);
        }
        constexpr void skip_whitespace(std::false_type) {}

        const char* begin_{};
        const char* ptr_{};
        const char* end_{};

        match_result<TokenSpec> last_result_;
    };
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_TOKENIZER_HPP_INCLUDED
