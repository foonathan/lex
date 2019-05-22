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
        template <class TokenSpec, class LiteralTokens>
        struct build_trie_impl;

        template <class TokenSpec>
        struct build_trie_impl<TokenSpec, type_list<>>
        {
            using type = typename trie<TokenSpec>::empty;
        };

        template <class TokenSpec, typename Head, typename... Tail>
        struct build_trie_impl<TokenSpec, type_list<Head, Tail...>>
        {
            using base = typename build_trie_impl<TokenSpec, type_list<Tail...>>::type;
            using type = typename trie<TokenSpec>::template insert_literal_str<
                base, token_kind<TokenSpec>(Head{}).get(), literal_token_type<Head>>;
        };

        template <class TokenSpec, class LiteralTokens>
        using literal_trie =
            typename build_trie_impl<TokenSpec, typename LiteralTokens::list>::type;

        //=== keyword and identifier trie ===//
        template <class TokenSpec, class Identifiers, class Keywords>
        struct keyword_identifier_matcher
        {
            static_assert(Identifiers::size <= 1, "at most one identifier_token token is allowed");
        };

        template <class TokenSpec, class Identifier, class Keywords>
        struct keyword_identifier_matcher<TokenSpec, type_list<Identifier>, Keywords>
        {
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
                auto keyword = literal_trie<TokenSpec, Keywords>::try_match(identifier_begin,
                                                                            identifier_end);
                if (keyword.is_matched() && keyword.bump == identifier.bump)
                    // we've matched a keyword and it isn't a prefix but the whole string
                    return keyword;
                else
                    // didn't match keyword or it was only a prefix
                    return identifier;
            }
        };

        template <class TokenSpec, class Keywords>
        struct keyword_identifier_matcher<TokenSpec, type_list<>, Keywords>
        {
            static_assert(Keywords::size == 0, "keyword tokens require an identifier_token token");

            static constexpr bool is_conflicting_literal(token_kind<TokenSpec>) noexcept
            {
                return false;
            }

            static constexpr match_result<TokenSpec> try_match(const char*, const char*) noexcept
            {
                // no identifier rule, so will never match
                return match_result<TokenSpec>::unmatched();
            }
        };

        //=== try_match ===//
        template <class TokenSpec, class Root, class Rules>
        struct insert_rules
        {
            using type = Root;
        };

        template <class TokenSpec, class Root, class Head, class... Tail>
        struct insert_rules<TokenSpec, Root, type_list<Head, Tail...>>
        {
            using with_head = typename trie<TokenSpec>::template insert_rule<Root, Head>;
            using type      = typename insert_rules<TokenSpec, with_head, type_list<Tail...>>::type;
        };

        template <class TokenSpec>
        struct build_trie
        {
            // split the tokens
            using identifiers = keep_if<TokenSpec, is_identifier_token>;
            using keywords    = keep_if<TokenSpec, is_keyword_token>;
            using rule_tokens = keep_if<TokenSpec, is_non_identifier_rule_token>;
            using literals    = keep_if<TokenSpec, is_non_keyword_literal_token>;

            // start with the literal trie
            using trie0 = detail::literal_trie<TokenSpec, literals>;
            // insert all rule tokens
            using trie1 = typename insert_rules<TokenSpec, trie0, rule_tokens>::type;
            // insert the keyword identifier rule
            using keyword_identifier_matcher
                = detail::keyword_identifier_matcher<TokenSpec, identifiers, keywords>;
            using trie2 =
                typename detail::trie<TokenSpec>::template insert_rule<trie1,
                                                                       keyword_identifier_matcher>;
        };

        template <class TokenSpec>
        using token_spec_trie = typename build_trie<TokenSpec>::trie2;
    } // namespace detail

    template <class TokenSpec>
    class tokenizer
    {
        using trie = detail::token_spec_trie<TokenSpec>;
        static_assert(detail::all_of<TokenSpec, is_token>::value,
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

            using any_whitespace = detail::any_of<TokenSpec, is_whitespace_token>;
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
