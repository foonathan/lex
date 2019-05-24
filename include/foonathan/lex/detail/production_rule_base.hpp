// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_PRODUCTION_RULE_BASE_HPP_INCLUDED
#define FOONATHAN_LEX_PRODUCTION_RULE_BASE_HPP_INCLUDED

#include <boost/mp11/algorithm.hpp>

#include <foonathan/lex/grammar.hpp>
#include <foonathan/lex/parse_error.hpp>
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
            namespace mp = boost::mp11;

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

            template <class... Tokens>
            struct peek_token // empty
            {
                template <class TokenSpec>
                static constexpr bool is(const tokenizer<TokenSpec>&)
                {
                    return false;
                }
            };
            template <class Head, class... Tail>
            struct peek_token<Head, Tail...>
            {
                template <class TokenSpec>
                static constexpr bool is(const tokenizer<TokenSpec>& tokenizer)
                {
                    return tokenizer.peek().is(Head{}) || peek_token<Tail...>::is(tokenizer);
                }
            };
            template <class... Tail>
            struct peek_token<any_token, Tail...>
            {
                template <class TokenSpec>
                static constexpr bool is(const tokenizer<TokenSpec>&)
                {
                    return true;
                }
            };

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

            template <class Func>
            struct forward_callback
            {
                Func& f;

                template <class F = Func, class P>
                auto result_of(P) -> decltype(std::declval<F>().result_of(std::declval<P>()));

                template <typename... Args>
                constexpr auto production(Args&&... args) const
                    -> decltype(f.production(static_cast<Args&&>(args)...))
                {
                    return f.production(static_cast<Args&&>(args)...);
                }
                template <class F = Func, typename... Args>
                constexpr auto finish(Args&&... args) const
                    -> decltype(std::declval<F>().finish(static_cast<Args&&>(args)...))
                {
                    return f.finish(static_cast<Args&&>(args)...);
                }
                template <typename... Args>
                constexpr auto error(Args&&... args) const
                    -> decltype(f.error(static_cast<Args&&>(args)...))
                {
                    return f.error(static_cast<Args&&>(args)...);
                }
            };

            /// A parsing callback that ignores all arguments.
            struct ignore_callback
            {
                template <typename... Args>
                constexpr void production(Args&&...) const
                {}
                template <typename... Args>
                constexpr void error(Args&&...) const
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

            /// Whether not the parser would parse the input.
            template <class Parser, class TokenSpec>
            constexpr auto is_parsed(tokenizer<TokenSpec> tokenizer)
            {
                ignore_callback callback;
                return Parser::parse(tokenizer, callback).is_success();
            }

            /// Whether or not the rule would parse the input.
            template <class Rule, class TokenSpec>
            constexpr auto is_rule_parsed(const tokenizer<TokenSpec>& tokenizer)
            {
                return is_parsed<parser_for<Rule, test_parser<TokenSpec>>>(tokenizer);
            }

            /// A parsing callback that ignores errors, but forwards everything else.
            template <class Func>
            struct ignore_error_callback : forward_callback<Func>
            {
                constexpr ignore_error_callback(Func& f) : forward_callback<Func>{f} {}

                template <typename... Args>
                constexpr void error(Args&&...) const
                {}
            };

            /// Tries to parse using the parser.
            /// If it fails, no error is reported.
            template <class Parser, class TokenSpec, class Func, typename... Args>
            constexpr auto try_parse(tokenizer<TokenSpec>& tokenizer, Func& f, Args&&... args)
            {
                auto cur_state = tokenizer;

                ignore_error_callback<Func> callback{f};
                auto result = Parser::parse(tokenizer, callback, static_cast<Args&&>(args)...);

                // reset tokenizer if necessary
                if (result.is_unmatched())
                    tokenizer = cur_state;

                return result;
            }
        } // namespace detail
    }     // namespace production_rule
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_PRODUCTION_RULE_BASE_HPP_INCLUDED
