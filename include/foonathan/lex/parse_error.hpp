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
    //=== unexpected_token ===//
    /// While trying to parse `Production`, it expected `Token` but a different one was next.
    template <class Grammar, class Production = void, class Token = void>
    struct unexpected_token;

    template <class Grammar>
    struct unexpected_token<Grammar, void, void>
    {
        production_kind<Grammar>                 production;
        token_kind<typename Grammar::token_spec> expected;

        template <class Production, class Token>
        constexpr unexpected_token(Production p, Token t) noexcept : production(p), expected(t)
        {}
    };

    template <class Grammar, class Production, class Token>
    struct unexpected_token : unexpected_token<Grammar>
    {
        constexpr unexpected_token(Production p, Token t) noexcept : unexpected_token<Grammar>(p, t)
        {}
    };

    //=== exhausted_token_choice ===//
    /// While trying to parse `Production`, it expected one of `Alternatives` but none were next.
    template <class Grammar, class Production = void, class... Alternatives>
    struct exhausted_token_choice;

    template <class Grammar>
    struct exhausted_token_choice<Grammar, void>
    {
        production_kind<Grammar>                        production;
        const token_kind<typename Grammar::token_spec>* alternative_array;
        std::size_t                                     number_of_alternatives;

        template <class Production, std::size_t N>
        constexpr exhausted_token_choice(
            Production p, const token_kind<typename Grammar::token_spec> (&array)[N]) noexcept
        : production(p), alternative_array(array), number_of_alternatives(N)
        {}

        constexpr auto begin() const noexcept
        {
            return alternative_array;
        }

        constexpr auto end() const noexcept
        {
            return alternative_array + number_of_alternatives;
        }
    };

    template <class Grammar, class Production, class... Alternatives>
    struct exhausted_token_choice : exhausted_token_choice<Grammar>
    {
        template <std::size_t N>
        constexpr exhausted_token_choice(
            Production p, const token_kind<typename Grammar::token_spec> (&array)[N]) noexcept
        : exhausted_token_choice<Grammar>(p, array)
        {}
    };

    //=== exhausted_choice ===>
    /// While trying to parse `Production`, it expected one of its alternatives, but failed.
    template <class Grammar, class Production = void>
    struct exhausted_choice;

    template <class Grammar>
    struct exhausted_choice<Grammar, void>
    {
        production_kind<Grammar> production;

        template <class Production>
        constexpr exhausted_choice(Production p) noexcept : production(p)
        {}
    };

    template <class Grammar, class Production>
    struct exhausted_choice : exhausted_choice<Grammar>
    {
        constexpr exhausted_choice(Production p) noexcept : exhausted_choice<Grammar>(p) {}
    };

    //=== illegal_operator_chain ===//
    /// While trying to parse `OperatorProduction`, an operator was chained even though it was not
    /// allowed to.
    template <class Grammar, class OperatorProduction = void>
    struct illegal_operator_chain;

    template <class Grammar>
    struct illegal_operator_chain<Grammar, void>
    {
        production_kind<Grammar> production;

        template <class Production>
        constexpr illegal_operator_chain(Production p) noexcept : production(p)
        {}
    };

    template <class Grammar, class Production>
    struct illegal_operator_chain : illegal_operator_chain<Grammar>
    {
        constexpr illegal_operator_chain(Production p) noexcept : illegal_operator_chain<Grammar>(p)
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
