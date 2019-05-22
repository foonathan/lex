// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_MATCH_RESULT_HPP_INCLUDED
#define FOONATHAN_LEX_MATCH_RESULT_HPP_INCLUDED

#include <foonathan/lex/detail/assert.hpp>
#include <foonathan/lex/token_kind.hpp>

namespace foonathan
{
namespace lex
{
    template <class TokenSpec>
    struct match_result
    {
        token_kind<TokenSpec> kind;
        std::size_t           bump;

        match_result() = delete;

        static constexpr auto unmatched() noexcept
        {
            return match_result<TokenSpec>({}, 0);
        }

        static constexpr auto error(std::size_t bump) noexcept
        {
            FOONATHAN_LEX_PRECONDITION(bump > 0, "bump must not be 0");
            return match_result<TokenSpec>({}, bump);
        }

        static constexpr auto success(token_kind<TokenSpec> kind, std::size_t bump) noexcept
        {
            FOONATHAN_LEX_PRECONDITION(bump > 0, "bump must not be 0");
            FOONATHAN_LEX_PRECONDITION(!kind.is(error_token{}) && !kind.is(eof_token{}),
                                       "use eof() or error() to match a special token");
            return match_result<TokenSpec>(kind, bump);
        }

        static constexpr auto eof() noexcept
        {
            return match_result<TokenSpec>(eof_token{}, 0);
        }

        constexpr bool is_unmatched() const noexcept
        {
            return !is_eof() && bump == 0;
        }

        constexpr bool is_error() const noexcept
        {
            return bump > 0 && !kind;
        }

        constexpr bool is_success() const noexcept
        {
            return bump > 0 && kind;
        }

        constexpr bool is_eof() const noexcept
        {
            return kind.is(eof_token{});
        }

        constexpr bool is_matched() const noexcept
        {
            return !is_unmatched();
        }

    private:
        explicit constexpr match_result(token_kind<TokenSpec> kind, std::size_t bump) noexcept
        : kind(kind), bump(bump)
        {}
    };
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_MATCH_RESULT_HPP_INCLUDED
