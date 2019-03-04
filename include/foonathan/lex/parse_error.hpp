// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_PARSE_ERROR_HPP_INCLUDED
#define FOONATHAN_LEX_PARSE_ERROR_HPP_INCLUDED

#include <foonathan/lex/production_kind.hpp>
#include <foonathan/lex/token_kind.hpp>

namespace foonathan
{
namespace lex
{
    /// While trying to parse `Production`, it expected `Token` but a different one occurred.
    template <class Grammar, class Production = void, class Token = void>
    struct unexpected_token_error;

    template <class Grammar>
    struct unexpected_token_error<Grammar, void, void>
    {
        production_kind<Grammar>                 production;
        token_kind<typename Grammar::token_spec> expected;

        template <class Production, class Token>
        constexpr unexpected_token_error(Production p, Token t) noexcept
        : production(p), expected(t)
        {}
    };

    template <class Grammar, class Production, class Token>
    struct unexpected_token_error : unexpected_token_error<Grammar>
    {
        constexpr unexpected_token_error(Production p, Token t) noexcept
        : unexpected_token_error<Grammar>(p, t)
        {}
    };

    namespace detail
    {
        template <class Func, class Error, class Tokenizer>
        constexpr auto report_error_impl(int, Func&& f, Error e, const Tokenizer& tokenizer)
            -> decltype(f(e, tokenizer))
        {
            return f(e, tokenizer);
        }

        template <class Error>
        constexpr bool required_error_type = false;
        template <class Error>
        struct missing_error_handler
        {
            static_assert(required_error_type<Error>, "missing error handler");
        };

        template <class Func, class Error, class Tokenizer>
        constexpr auto report_error_impl(short, Func, Error, const Tokenizer&)
        {
            return missing_error_handler<Error>{};
        }

        template <class Func, class Error, class Tokenizer>
        constexpr void report_error(Func&& f, Error e, const Tokenizer& tokenizer)
        {
            report_error_impl(0, f, e, tokenizer);
        }
    } // namespace detail
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_PARSE_ERROR_HPP_INCLUDED
