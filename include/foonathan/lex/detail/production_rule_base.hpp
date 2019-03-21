// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_PRODUCTION_RULE_BASE_HPP_INCLUDED
#define FOONATHAN_LEX_PRODUCTION_RULE_BASE_HPP_INCLUDED

#include <foonathan/lex/grammar.hpp>
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

            //=== token check ===//
            /// Tag type to mark any token.
            struct any_token
            {};

            template <class TokenSpec>
            constexpr bool peek_token_is(lex::detail::type_list<>, const tokenizer<TokenSpec>&)
            {
                return false;
            }

            template <class Head, class... Tail, class TokenSpec>
            constexpr bool peek_token_is(lex::detail::type_list<Head, Tail...>,
                                         const tokenizer<TokenSpec>& tokenizer)
            {
                if (tokenizer.peek().is(Head{}))
                    return true;
                else
                    return peek_token_is(lex::detail::type_list<Tail...>{}, tokenizer);
            }

            template <class... Tail, class TokenSpec>
            constexpr bool peek_token_is(lex::detail::type_list<any_token, Tail...>,
                                         const tokenizer<TokenSpec>&)
            {
                return true;
            }

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

            /// A parsing callback that ignores all arguments.
            struct ignore_callback
            {
                template <typename... Args>
                constexpr void operator()(Args&&...) const
                {}
            };

            /// A parser that just returns success or not if it matched.
            template <class TokenSpec>
            struct test_parser
            {
                // actual production type does not matter here
                using grammar = lex::grammar<TokenSpec, int>;
                using tlp     = int;

                template <class Func, typename... Args>
                static constexpr auto parse(tokenizer<TokenSpec>&, Func&, Args&&...)
                {
                    return lex::parse_result<void>::success();
                }
            };
        } // namespace detail
    }     // namespace production_rule
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_PRODUCTION_RULE_BASE_HPP_INCLUDED
