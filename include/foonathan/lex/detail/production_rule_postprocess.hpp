// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_PRODUCTION_RULE_POSTPROCESS_HPP_INCLUDED
#define FOONATHAN_LEX_PRODUCTION_RULE_POSTPROCESS_HPP_INCLUDED

#include <foonathan/lex/detail/production_rule_production.hpp>
#include <foonathan/lex/detail/production_rule_token.hpp>

namespace foonathan
{
namespace lex
{
    namespace production_rule
    {
        namespace detail
        {
            //=== has_subrule ===//
            template <class Rule, class SubRule>
            struct has_subrule : std::false_type
            {};

            template <class Rule>
            struct has_subrule<Rule, Rule> : std::true_type
            {};

            template <class... Rules, class SubRule>
            struct has_subrule<sequence<Rules...>, SubRule>
            {
                template <class R>
                using predicate = has_subrule<R, SubRule>;

                static constexpr auto value
                    = lex::detail::any_of<lex::detail::type_list<Rules...>, predicate>::value;
            };

            template <class P, class R, class SubRule>
            struct has_subrule<choice_alternative<P, R>, SubRule>
            {
                static constexpr auto value
                    = has_subrule<P, SubRule>::value || has_subrule<R, SubRule>::value;
            };

            template <class... Rules, class SubRule>
            struct has_subrule<choice<Rules...>, SubRule>
            {
                template <class R>
                using predicate = has_subrule<R, SubRule>;

                static constexpr auto value
                    = lex::detail::any_of<lex::detail::type_list<Rules...>, predicate>::value;
            };

            template <class C, class T, class SubRule>
            struct has_subrule<left_recursion_choice<C, T>, SubRule>
            {
                static constexpr auto value
                    = has_subrule<C, SubRule>::value || has_subrule<T, SubRule>::value;
            };

            //=== recursion in peek rules ===//
            template <class TLP, class Rule>
            struct detect_peek_recursion
            {
                using type = Rule;
            };

            template <class TLP, class PeekRule, class Rule>
            struct detect_peek_recursion<TLP, choice_alternative<PeekRule, Rule>>
            {
                static_assert(!has_subrule<PeekRule, production<TLP>>::value
                                  && !has_subrule<PeekRule, recurse_production<TLP>>::value,
                              "recursion in peek rule is not allowed");
                using type = choice_alternative<PeekRule, Rule>;
            };

            template <class TLP, class... C>
            struct detect_peek_recursion<TLP, choice<C...>>
            {
                using type = choice<typename detect_peek_recursion<TLP, C>::type...>;
            };

            template <class TLP, class C, class T>
            struct detect_peek_recursion<TLP, left_recursion_choice<C, T>>
            {
                using type = left_recursion_choice<typename detect_peek_recursion<TLP, C>::type,
                                                   typename detect_peek_recursion<TLP, T>::type>;
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
                using tail = typename has_left_recursion<TLP, Rule>::tail;
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
                using tail = typename has_left_recursion<TLP, Rule>::tail;
                static_assert(!std::is_same<tail, sequence<>>::value,
                              "infinite recursion detected");
                using type = left_recursion_choice<choice<Alternatives...>, tail>;
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

            //=== postprocess ===//
            template <class TLP, class Rule>
            using postprocess0 = typename eliminate_left_recursion<TLP, Rule>::type;
            template <class TLP, class Rule>
            using postprocess = typename detect_peek_recursion<TLP, postprocess0<TLP, Rule>>::type;
        } // namespace detail
    }     // namespace production_rule
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_PRODUCTION_RULE_POSTPROCESS_HPP_INCLUDED
