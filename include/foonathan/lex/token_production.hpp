// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_TOKEN_PRODUCTION_HPP_INCLUDED
#define FOONATHAN_LEX_TOKEN_PRODUCTION_HPP_INCLUDED

#include <foonathan/lex/grammar.hpp>
#include <foonathan/lex/parse_error.hpp>
#include <foonathan/lex/parse_result.hpp>
#include <foonathan/lex/tokenizer.hpp>

namespace foonathan
{
namespace lex
{
    /// A production that just consists of a single token.
    ///
    /// It is a CRTP base class, where `Derived` is the actual type of the production.
    /// `Grammar` is the grammar containing all productions, and `Token` is the token the production
    /// consists of.
    template <class Derived, class Grammar, class Token>
    struct token_production : detail::base_production
    {
        template <class Func>
        static constexpr auto parse(tokenizer<typename Grammar::token_spec>& tokenizer, Func&& f)
            -> decltype(lex::detail::apply_parse_result(f, std::declval<Derived>(),
                                                        std::declval<lex::static_token<Token>>()))
        {
            auto token = tokenizer.peek();
            if (token.is(Token{}))
            {
                tokenizer.bump();
                return lex::detail::apply_parse_result(f, Derived{},
                                                       lex::static_token<Token>(token));
            }
            else
            {
                auto error = lex::unexpected_token<Grammar, Derived, Token>(Derived{}, Token{});
                lex::detail::report_error(f, error, tokenizer);
                return {};
            }
        }
    };
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_TOKEN_PRODUCTION_HPP_INCLUDED
