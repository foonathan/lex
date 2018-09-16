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

    /// Tokenizes a character range according the token specification.
    ///
    /// It will tokenize it by trying each of the specified tokens in an arbitrary order.
    /// The token that matches will be stored and the position advanced.
    /// If no token matched, it will store an error token and advance to the next character.
    /// If a token rule matched an error token, it will be transparently forwarded.
    ///
    /// It will only store one token in memory.
    /// Parsers requiring look ahead can be implemented by resetting the tokenizer to an earlier
    /// position, if necessary.
    template <class TokenSpec>
    class tokenizer
    {
    public:
        //=== constructors ===//
        /// \effects Creates a tokenizer that will tokenize the range `[ptr, ptr + size)`.
        explicit constexpr tokenizer(const char* ptr, std::size_t size) noexcept
        : tokenizer(ptr, ptr + size)
        {}

        /// \effects Creates a tokenizer that will tokenize the range `[begin, end)`.
        explicit constexpr tokenizer(const char* begin, const char* end)
        : begin_(begin), ptr_(begin), end_(end)
        {
            bump();
        }

        /// \effects Creates a tokenizer that will tokenize the given array.
        template <std::size_t N>
        explicit constexpr tokenizer(const char (&array)[N]) : tokenizer(array, array + N)
        {}

        //=== tokenizer functions ===//
        /// \returns The current token.
        constexpr token<TokenSpec> peek() const noexcept
        {
            return cur_;
        }

        /// \returns Whether or not EOF was reached.
        /// If this becomes `true` for the first time, the next call to `bump()` will store the EOF
        /// token. Then it will still return `true` but `peek()` will always return EOF and `bump()`
        /// has no effect anymore.
        constexpr bool is_eof() const noexcept
        {
            return ptr_ == end_;
        }

        /// \returns Whether or not the current token is an error token.
        constexpr bool is_error() const noexcept
        {
            return peek().is_error();
        }

        /// Returns and advances the token.
        /// \effects Consumes the current token by calling `bump()`.
        /// \returns The current token, before it was consumed.
        constexpr token<TokenSpec> get() noexcept
        {
            auto result = peek();
            bump();
            return result;
        }

        /// \effects Consumes the current token, `peek()` will then return the next token.
        /// If `is_eof() == true` when calling the function it will replace the current token with
        /// EOF.
        constexpr void bump() noexcept
        {
            auto offset = static_cast<std::size_t>(ptr_ - begin_);

            if (ptr_ == end_)
                cur_ = token<TokenSpec>(token_kind<TokenSpec>(eof{}), ptr_, 0);
            else
            {
                using matcher_list = detail::matcher_list<TokenSpec>;

                auto result
                    = detail::try_match_impl<TokenSpec, matcher_list>::try_match(ptr_, end_);
                // TODO: assert result.is_matched() && range of ptr_
                cur_ = token<TokenSpec>(result.kind, ptr_, result.bump);
                ptr_ += result.bump;
            }
        }

        /// \effects Resets the tokenizer to the specified position.
        /// The next call to `bump()` will parse a token starting there.
        constexpr void reset(const char* position) noexcept
        {
            // TODO: assert
            ptr_ = position;
        }

        //=== getters ===//
        /// \returns The beginning of the character range.
        constexpr const char* begin_ptr() const noexcept
        {
            return begin_;
        }

        /// \returns The current position in the character range.
        /// After `bump()` is called, `peek()` will return the token starting at that position.
        constexpr const char* current_ptr() const noexcept
        {
            return ptr_;
        }

        /// \returns One past the end of the character range.
        constexpr const char* end_ptr() const noexcept
        {
            return end_;
        }

    private:
        const char* begin_{};
        const char* ptr_{};
        const char* end_{};

        token<TokenSpec> cur_;
    };
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_TOKENIZER_HPP_INCLUDED
