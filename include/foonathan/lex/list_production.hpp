// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_LIST_PRODUCTION_HPP_INCLUDED
#define FOONATHAN_LEX_LIST_PRODUCTION_HPP_INCLUDED

#include <foonathan/lex/grammar.hpp>
#include <foonathan/lex/parse_error.hpp>
#include <foonathan/lex/parse_result.hpp>
#include <foonathan/lex/tokenizer.hpp>

namespace foonathan
{
namespace lex
{
    template <class Derived, class Grammar, class Production, class Separator>
    struct list_production : detail::base_production
    {
        static_assert(is_production<Production>::value, "list element must be a production");
        static_assert(is_token<Separator>::value, "list separator must be a token");

        template <class Func>
        static constexpr auto parse(tokenizer<typename Grammar::token_spec>& tokenizer, Func&& f)
            -> decltype(lex::detail::apply_parse_result(
                f, std::declval<Derived>(),
                Production::parse(tokenizer, f).template value_or_tag<Production>()))
        {
            auto elem = Production::parse(tokenizer, f);
            if (elem.is_unmatched())
                return {};

            auto result = lex::detail::apply_parse_result(f, Derived{},
                                                          elem.template value_or_tag<Production>());

            while (tokenizer.peek().is(Separator{}))
            {
                tokenizer.bump();

                elem = Production::parse(tokenizer, f);
                if (elem.is_unmatched())
                    return {};

                result = lex::detail::apply_parse_result(f, Derived{},
                                                         result.template value_or_tag<Derived>(),
                                                         elem.template value_or_tag<Production>());
            }

            return result;
        }
    };
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_LIST_PRODUCTION_HPP_INCLUDED
