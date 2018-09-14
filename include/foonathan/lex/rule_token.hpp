// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_RULE_TOKEN_HPP_INCLUDED
#define FOONATHAN_LEX_RULE_TOKEN_HPP_INCLUDED

#include <foonathan/lex/token.hpp>

namespace foonathan
{
    namespace lex
    {
        /// The result of a [lex::rule_token]().
        template <class TokenSpec>
        struct parse_rule_result
        {
            token_kind<TokenSpec> kind; //< The kind of token that was parsed.
            std::size_t           bump; //< How many characters were consumed.

            /// \effects Creates a result that didn't match anything.
            explicit constexpr parse_rule_result() noexcept : kind(), bump(0) {}

            /// \effects Creates a failed result containing an error consuming the given amount of characters.
            explicit constexpr parse_rule_result(std::size_t bump) noexcept : kind(), bump(bump)
            {
                // TODO: assert bump is not zero
            }

            /// \effects Creates a successful result that parsed the given token.
            explicit constexpr parse_rule_result(token_kind<TokenSpec> kind, std::size_t bump)
            : kind(kind), bump(bump)
            {
                // TODO: assert bump is not zero and kind is not error
            }

            /// \returns Whether or not anything was matched.
            constexpr bool is_matched() const noexcept
            {
                return bump != 0;
            }

            /// \returns Whether or not the result is an error.
            constexpr bool is_error() const noexcept
            {
                return is_matched() && !kind;
            }

            /// \returns Whether or not the result is a success.
            constexpr bool is_success() const noexcept
            {
                return is_matched() && kind;
            }
        };

        /// A token that follows a complex parsing rule.
        ///
        /// It must provide a function `static parse_rule_result try_match(const char* cur, const char* end) noexcept;`.
        /// It is invoked with a pointer to the current character and to the end.
        /// There will always be at least one character.
        /// This function tries to parse a token and reports success or failure.
        /// If it didn't match anything, the next rule is tried.
        /// If it was an error, an error token is created.
        /// If it was a success, the correct token is created.
        template <class TokenSpec>
        struct rule_token
        {
        };
    } // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_RULE_TOKEN_HPP_INCLUDED
