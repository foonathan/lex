// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_MATCH_RESULT_HPP_INCLUDED
#define FOONATHAN_LEX_MATCH_RESULT_HPP_INCLUDED

#include <foonathan/lex/token_kind.hpp>

namespace foonathan
{
namespace lex
{
    /// The result of a [lex::rule_token]().
    template <class TokenSpec>
    struct match_result
    {
        token_kind<TokenSpec> kind; //< The kind of token that was parsed.
        std::size_t           bump; //< How many characters were consumed.

        /// \effects Creates a result that didn't match anything.
        explicit constexpr match_result() noexcept : kind(), bump(0) {}

        /// \effects Creates a failed result containing an error consuming the given amount of
        /// characters.
        explicit constexpr match_result(std::size_t bump) noexcept : kind(), bump(bump)
        {
            // TODO: assert bump is not zero
        }

        /// \effects Creates a successful result that parsed the given token.
        explicit constexpr match_result(token_kind<TokenSpec> kind, std::size_t bump)
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

} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_MATCH_RESULT_HPP_INCLUDED
