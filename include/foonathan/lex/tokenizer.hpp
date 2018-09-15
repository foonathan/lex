// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_TOKENIZER_HPP_INCLUDED
#define FOONATHAN_LEX_TOKENIZER_HPP_INCLUDED

#include <foonathan/lex/identifier.hpp>
#include <foonathan/lex/literal_token.hpp>
#include <foonathan/lex/rule_token.hpp>
#include <foonathan/lex/token.hpp>

namespace foonathan
{
namespace lex
{
    namespace detail
    {
        //=== matcher_list ===//
        template <class TokenSpec>
        struct get_matcher_list
        {
            // the rule tokens are all matcher already
            using rule_matchers = keep_if<TokenSpec, is_non_identifier_rule_token>;

            // the identifier matcher
            using identifiers = keep_if<TokenSpec, is_identifier>;
            using keywords    = keep_if<TokenSpec, is_keyword>;
            using keyword_identifier_matcher
                = detail::keyword_identifier_matcher<TokenSpec, identifiers, keywords>;

            // use one literal matcher to match all literals
            using literal_matcher
                = detail::literal_matcher<TokenSpec,
                                          keep_if<TokenSpec, is_non_keyword_literal_token>>;

            // concatenate all
            using type = concat<rule_matchers, keyword_identifier_matcher, literal_matcher>;
        };

        template <class TokenSpec>
        using matcher_list = typename get_matcher_list<TokenSpec>::type;

        //=== try_match_impl ===//
        /// Calls ::try_match() for every matcher until one is found that matches.
        template <class TokenSpec, class MatcherList>
        struct try_match_impl;

        template <class TokenSpec>
        struct try_match_impl<TokenSpec, type_list<>>
        {
            static constexpr match_result<TokenSpec> try_match(const char*, const char*) noexcept
            {
                // nothing matched, create an error
                return match_result<TokenSpec>(1);
            }
        };

        template <class TokenSpec, class Head, class... Tail>
        struct try_match_impl<TokenSpec, type_list<Head, Tail...>>
        {
            using tail_match = try_match_impl<TokenSpec, type_list<Tail...>>;

            static constexpr match_result<TokenSpec> try_match(const char* str,
                                                               const char* end) noexcept
            {
                auto result = Head::try_match(str, end);
                if (result.is_matched())
                    return result;
                else
                    return tail_match::try_match(str, end);
            }
        };
    } // namespace detail

    template <class TokenSpec>
    class tokenizer
    {
        using matcher_list = detail::matcher_list<TokenSpec>;

    public:
        explicit constexpr tokenizer(const char* ptr, std::size_t size)
        : begin_(ptr), ptr_(ptr), end_(ptr + size)
        {
            bump();
        }

        /// \returns Whether or not EOF was reached.
        constexpr bool is_eof() const noexcept
        {
            return ptr_ == end_;
        }

        /// \returns The current token.
        constexpr token<TokenSpec> peek() const noexcept
        {
            return cur_;
        }

        /// \effects Consumes the current token by calling `bump()`.
        /// \returns The current token, before it was consumed.
        constexpr token<TokenSpec> get() noexcept
        {
            auto result = peek();
            bump();
            return result;
        }

        /// \effects Consumes the current token.
        /// If `is_eof() == true`, does nothing.
        constexpr void bump() noexcept
        {
            if (ptr_ == end_)
                return;

            auto result = detail::try_match_impl<TokenSpec, matcher_list>::try_match(ptr_, end_);
            // TODO: assert result.is_matched() && range of ptr_
            cur_ = token<TokenSpec>(result.kind, token_spelling(ptr_, result.bump),
                                    static_cast<std::size_t>(ptr_ - begin_));
            ptr_ += result.bump;
        }

    private:
        const char* begin_;
        const char* ptr_;
        const char* end_;

        token<TokenSpec> cur_;
    };
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_TOKENIZER_HPP_INCLUDED
