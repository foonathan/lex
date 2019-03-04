// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_PRODUCTION_RULE_BASE_HPP_INCLUDED
#define FOONATHAN_LEX_PRODUCTION_RULE_BASE_HPP_INCLUDED

#include <foonathan/lex/parse_result.hpp>
#include <foonathan/lex/tokenizer.hpp>

namespace foonathan
{
namespace lex
{
    namespace production_rule
    {
        struct base_rule
        {};

        namespace detail
        {
            //=== parser_for ===//
            template <class... Rules>
            struct parser_for_impl;
            template <class Cont>
            struct parser_for_impl<Cont>
            {
                using type = Cont;
            };
            template <class Head, class... Tail>
            struct parser_for_impl<Head, Tail...>
            {
                using type =
                    typename Head::template parser<typename parser_for_impl<Tail...>::type>;
            };
            template <class... Rules>
            using parser_for = typename parser_for_impl<Rules...>::type;

            //=== parser implementations ===//
            /// The final parser that invokes the callback.
            template <class Grammar, class TLP>
            struct final_parser
            {
                using grammar = Grammar;
                using tlp     = TLP;

                template <class TokenSpec, class Func, typename... Args>
                static constexpr auto parse(tokenizer<TokenSpec>&, Func& f, Args&&... args)
                {
                    return lex::detail::apply_parse_result(f, TLP{}, static_cast<Args&&>(args)...);
                }
            };
        } // namespace detail
    }     // namespace production_rule
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_PRODUCTION_RULE_BASE_HPP_INCLUDED
