// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_PRODUCTION_RULE_PRODUCTION_HPP_INCLUDED
#define FOONATHAN_LEX_PRODUCTION_RULE_PRODUCTION_HPP_INCLUDED

#include <foonathan/lex/detail/production_rule_base.hpp>
#include <foonathan/lex/parse_error.hpp>

namespace foonathan
{
namespace lex
{
    namespace production_rule
    {
        namespace detail
        {
            //=== traits ===//
            struct base_choice_rule : base_rule
            {};
            template <typename T>
            struct is_choice_rule : std::is_base_of<base_choice_rule, T>
            {};

            //=== rules ===//
            template <class Production>
            constexpr bool for_production = false;

            template <class Func, class Production>
            struct missing_callback_result_of
            {
                static_assert(for_production<Production>, "need a callback_result_of overload");
            };

            template <class Production>
            struct recurse_production : base_rule
            {
                template <class Cont>
                struct parser : Cont
                {
                    static_assert(is_production<Production>::value,
                                  "only a production can be used in this context");

                    template <typename Func>
                    static constexpr auto callback_return_type(int, Func& f)
                        -> parse_result<decltype(f(callback_result_of<Production>{}))>;

                    template <typename Func>
                    static constexpr auto callback_return_type(short, Func&)
                    {
                        return missing_callback_result_of<Func, Production>{};
                    }

                    template <class TokenSpec, typename Func, typename... Args>
                    static constexpr auto parse(tokenizer<TokenSpec>& tokenizer, Func& f,
                                                Args&&... args)
                        -> decltype(Cont::parse(
                            tokenizer, f, static_cast<Args&&>(args)...,
                            callback_return_type(0, f).template value_or_tag<Production>()))
                    {
                        auto result = Production::parse(tokenizer, f);
                        if (result.is_success())
                            return Cont::parse(tokenizer, f, static_cast<Args&&>(args)...,
                                               result.template value_or_tag<Production>());
                        else
                            return {};
                    }
                };
            };

            template <class Production>
            struct production : base_rule
            {
                static_assert(is_production<Production>::value,
                              "only a production can be used in this context");

                template <class Cont>
                struct parser_impl : Cont
                {
                    template <class TokenSpec, typename Func, typename... Args>
                    static constexpr auto parse(tokenizer<TokenSpec>& tokenizer, Func& f,
                                                Args&&... args)
                        -> decltype(Cont::parse(
                            tokenizer, f, static_cast<Args&&>(args)...,
                            Production::parse(tokenizer, f).template value_or_tag<Production>()))
                    {
                        auto result = Production::parse(tokenizer, f);
                        if (result.is_success())
                            return Cont::parse(tokenizer, f, static_cast<Args&&>(args)...,
                                               result.template value_or_tag<Production>());
                        else
                            return {};
                    }
                };

                // if we're having direct recursion, we need the recursion parser, it handles the
                // issue with the return type
                template <class Cont>
                using parser
                    = std::conditional_t<std::is_same<Production, typename Cont::tlp>::value,
                                         parser_for<recurse_production<Production>, Cont>,
                                         parser_impl<Cont>>;
            };

            template <class... Rules>
            struct sequence : base_rule
            {
                static_assert(
                    lex::detail::none_of<lex::detail::type_list<Rules...>, is_choice_rule>::value,
                    "a choice cannot be composed further");

                template <class Cont>
                using parser = parser_for<Rules..., Cont>;
            };

            template <class PeekRule, class Rule>
            struct choice_alternative : base_choice_rule
            {
                using rule = Rule;

                template <class TokenSpec>
                static constexpr bool peek(tokenizer<TokenSpec> tokenizer)
                {
                    // use alternative if rule matched
                    ignore_callback f;
                    return parser_for<PeekRule, test_parser<TokenSpec>>::parse(tokenizer, f)
                        .is_success();
                }

                template <class Cont>
                using parser = parser_for<Rule, Cont>;
            };

            template <class... Choices>
            struct choice : base_choice_rule
            {
                template <class Cont>
                struct parser : Cont
                {
                    using grammar = typename Cont::grammar;
                    using tlp     = typename Cont::tlp;

                    template <class R, class TokenSpec, typename Func>
                    static constexpr R parse_impl(choice<>, tokenizer<TokenSpec>& tokenizer,
                                                  Func& f)
                    {
                        auto error = exhausted_choice<grammar, tlp>(tlp{});
                        lex::detail::report_error(f, error, tokenizer);
                        return {};
                    }
                    template <class R, class Head, class... Tail, class TokenSpec, typename Func>
                    static constexpr R parse_impl(choice<Head, Tail...>,
                                                  tokenizer<TokenSpec>& tokenizer, Func& f)
                    {
                        if (Head::peek(tokenizer))
                            return parser_for<Head, Cont>::parse(tokenizer, f);
                        else
                            return parse_impl<R>(choice<Tail...>{}, tokenizer, f);
                    }

                    template <class TokenSpec, typename Func>
                    static constexpr auto parse(tokenizer<TokenSpec>& tokenizer, Func& f)
                    {
                        using return_type = std::common_type_t<decltype(
                            parser_for<Choices, Cont>::parse(tokenizer, f))...>;
                        return parse_impl<return_type>(choice<Choices...>{}, tokenizer, f);
                    }
                };
            };

            /// Choice, then Tail as often as possible.
            template <class Choice, class Tail>
            struct left_recursion_choice : base_choice_rule
            {
                template <class Cont>
                struct parser : Cont
                {
                    using grammar = typename Cont::grammar;
                    using tlp     = typename Cont::tlp;

                    template <class TokenSpec, typename Func>
                    static constexpr auto parse(tokenizer<TokenSpec>& tokenizer, Func& f)
                    {
                        // parse choice, then invoke callback
                        using choice_parser = parser_for<Choice, final_parser<grammar, tlp>>;
                        auto result         = choice_parser::parse(tokenizer, f);
                        if (result.is_unmatched())
                            // error, didn't match at all
                            return result;

                        // try to parse result as often as possible
                        while (true)
                        {
                            auto next_result = try_parse<
                                parser_for<Tail, Cont>>(tokenizer, f,
                                                        result.template value_or_tag<tlp>());
                            if (next_result.is_unmatched())
                                break;
                            result = static_cast<decltype(next_result)&&>(next_result);
                        }

                        return result;
                    }
                };
            };

            //=== left recursion transformation ===//
            template <class TLP, class Rule>
            struct has_left_recursion : std::false_type
            {
                using tail = Rule;
            };
            template <class TLP>
            struct has_left_recursion<TLP, production<TLP>> : std::true_type
            {
                using tail = sequence<>;
            };
            template <class TLP>
            struct has_left_recursion<TLP, recurse_production<TLP>> : std::true_type
            {
                using tail = sequence<>;
            };
            template <class TLP, class Head, class... Tail>
            struct has_left_recursion<TLP, sequence<Head, Tail...>> : has_left_recursion<TLP, Head>
            {
                using tail = sequence<Tail...>;
            };
            template <class TLP, class PeekRule, class Rule>
            struct has_left_recursion<TLP, choice_alternative<PeekRule, Rule>>
            : has_left_recursion<TLP, Rule>
            {
                using tail
                    = choice_alternative<PeekRule, typename has_left_recursion<TLP, Rule>::tail>;
            };

            // RecursionList: all alternatives with left recursion
            // AlternativeList:: all alternatives without
            template <class TLP, class RecursionList, class AlternativeList>
            struct eliminate_left_recursion_impl;

            // no alternative has recursion
            template <class TLP, class... Alternatives>
            struct eliminate_left_recursion_impl<TLP, lex::detail::type_list<>,
                                                 lex::detail::type_list<Alternatives...>>
            {
                // just a choice of the alternatives
                using type = choice<Alternatives...>;
            };

            // single rule has recursion
            template <class TLP, class Rule, class... Alternatives>
            struct eliminate_left_recursion_impl<TLP, lex::detail::type_list<Rule>,
                                                 lex::detail::type_list<Alternatives...>>
            {
                // use left_recursion_choice, passing it the tail of that rule
                using type = left_recursion_choice<choice<Alternatives...>,
                                                   typename has_left_recursion<TLP, Rule>::tail>;
            };

            // multiple rules have recursion, not allowed
            template <class TLP, class... Rules, class... Alternatives>
            struct eliminate_left_recursion_impl<TLP, lex::detail::type_list<Rules...>,
                                                 lex::detail::type_list<Alternatives...>>
            {
                static_assert(sizeof...(Rules) == 1,
                              "only a single alternative may have left recursion");
            };

            template <class TLP, class Rule>
            struct eliminate_left_recursion
            {
                // base case, nothing needs to be done
                using type = Rule;
            };

            // only a choice, which must be a top-level rule, can be subject to left recursion,
            // (otherwise it is infinite anyway)
            template <class TLP, class... Alternatives>
            struct eliminate_left_recursion<TLP, choice<Alternatives...>>
            {
                using alternative_rules = lex::detail::type_list<Alternatives...>;

                template <class Rule>
                using has_recursion = has_left_recursion<TLP, Rule>;
                using left_recursion_alternatives
                    = lex::detail::keep_if<alternative_rules, has_recursion>;
                using other_alternatives = lex::detail::remove_if<alternative_rules, has_recursion>;

                using type =
                    typename eliminate_left_recursion_impl<TLP, left_recursion_alternatives,
                                                           other_alternatives>::type;
            };
        } // namespace detail
    }     // namespace production_rule
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_PRODUCTION_RULE_PRODUCTION_HPP_INCLUDED
