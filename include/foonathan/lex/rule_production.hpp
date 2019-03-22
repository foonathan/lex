// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_RULE_PRODUCTION_HPP_INCLUDED
#define FOONATHAN_LEX_RULE_PRODUCTION_HPP_INCLUDED

#include <foonathan/lex/detail/production_rule_production.hpp>
#include <foonathan/lex/detail/production_rule_token.hpp>
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
        namespace detail
        {
            //=== make_rule ===//
            template <typename T, typename = void>
            struct make_rule_impl
            {
                static_assert(std::is_base_of<base_rule, T>::value,
                              "invalid type in production rule DSL");
                using type = T;
            };

            template <class Token>
            struct make_rule_impl<Token, std::enable_if_t<is_token<Token>::value>>
            {
                using type = token<Token>;
            };

            template <class Production>
            struct make_rule_impl<Production, std::enable_if_t<is_production<Production>::value>>
            {
                using type = production<Production>;
            };

            template <typename T>
            using make_rule = typename make_rule_impl<T>::type;

            //=== DSL helpers ===//
            // tag type used as the result of silent
            template <class Rule>
            struct silent_return_type
            {};

            // if silent is called, we need to convert the tag type
            template <class Rule>
            struct make_rule_impl<silent_return_type<Rule>>
            {
                static_assert(is_token_rule<make_rule<Rule>>::value,
                              "only a token rule can be silenced");
                using type = typename make_rule<Rule>::silence;
            };
            // otherwise we have a function pointer and need to convert it
            template <class Rule>
            struct make_rule_impl<silent_return_type<Rule> (*)(Rule)>
            {
                static_assert(is_token_rule<make_rule<Rule>>::value,
                              "only a token rule can be silenced");
                using type = typename make_rule<Rule>::silence;
            };

            template <class Rule1, class Rule2>
            constexpr auto make_sequence(
                Rule1, Rule2,
                std::enable_if_t<is_token_rule<Rule1>::value && is_token_rule<Rule2>::value,
                                 int*> = 0)
            {
                return typename Rule1::template sequence_with<Rule2>{};
            }

            template <class Rule1, class Rule2>
            constexpr auto make_sequence(
                Rule1, Rule2,
                std::enable_if_t<!is_token_rule<Rule1>::value || !is_token_rule<Rule2>::value,
                                 short*> = 0)
            {
                return sequence<Rule1, Rule2>{};
            }
            template <class... Rules, class Rule2>
            constexpr auto make_sequence(sequence<Rules...>, Rule2)
            {
                return sequence<Rules..., Rule2>{};
            }

            template <class Rule1, class Rule2>
            constexpr auto make_token_choice(Rule1, Rule2)
            {
                static_assert(is_token_rule<Rule1>::value && is_token_rule<Rule2>::value,
                              "/ can note be used with productions, use | instead");
                return typename Rule1::template choice_with<Rule2>{};
            }

            template <class Rule>
            constexpr auto make_choice_alternative(Rule)
                -> std::enable_if_t<is_token_rule<Rule>::value, choice_alternative<Rule, Rule>>
            {
                return {};
            }
            template <class Rule>
            constexpr auto make_choice_alternative(Rule rule)
                -> std::enable_if_t<!is_token_rule<Rule>::value, Rule>
            {
                static_assert(is_choice_rule<Rule>::value,
                              "need to use >> to use this rule in a choice");
                return rule;
            }

            template <class Rule1, class Rule2>
            constexpr auto make_choice(Rule1, Rule2)
            {
                return choice<Rule1, Rule2>{};
            }
            template <class... Alternatives, class Rule2>
            constexpr auto make_choice(choice<Alternatives...>, Rule2)
            {
                return choice<Alternatives..., Rule2>{};
            }
        } // namespace detail

        //=== atomic rules ===//
        constexpr auto eof = detail::silent_token<eof_token>{};

        template <class Production>
        constexpr auto recurse = detail::recurse_production<Production>{};

        template <class Production>
        constexpr auto inline_ = detail::inline_production<Production>{};

        /// Can either be used as `silent<Token>` or `silent(Token{} + Token{})`.
        template <class Rule>
        constexpr auto silent(Rule rule)
        {
            (void)rule;
            return detail::silent_return_type<Rule>{};
        }

        //=== combinator rules ===//
        template <class Rule1, class Rule2>
        constexpr auto operator+(Rule1 rule1, Rule2 rule2) noexcept
        {
            (void)rule1, (void)rule2;
            return detail::make_sequence(detail::make_rule<Rule1>{}, detail::make_rule<Rule2>{});
        }

        template <class Rule1, class Rule2>
        constexpr auto operator/(Rule1 rule1, Rule2 rule2) noexcept
        {
            (void)rule1, (void)rule2;
            return detail::make_token_choice(detail::make_rule<Rule1>{},
                                             detail::make_rule<Rule2>{});
        }

        template <class Rule>
        constexpr auto opt(Rule rule) noexcept
        {
            (void)rule;
            return detail::make_token_choice(detail::make_rule<Rule>{}, detail::token_sequence<>{});
        }

        constexpr auto else_ = detail::token_sequence<>{};

        template <class PeekRule, class Rule>
        constexpr auto operator>>(PeekRule if_peek, Rule then)
        {
            (void)if_peek;
            (void)then;

            using peek_rule = detail::make_rule<PeekRule>;

            using rule = detail::make_rule<Rule>;
            return detail::choice_alternative<peek_rule, rule>{};
        }

        template <class Rule1, class Rule2>
        constexpr auto operator|(Rule1 rule1, Rule2 rule2)
        {
            (void)rule1;
            (void)rule2;

            return detail::make_choice(detail::make_choice_alternative(detail::make_rule<Rule1>{}),
                                       detail::make_choice_alternative(detail::make_rule<Rule2>{}));
        }
    } // namespace production_rule

    template <class Derived, class Grammar>
    class rule_production : public detail::base_production
    {
        template <class Func>
        static constexpr auto parse_impl(int, tokenizer<typename Grammar::token_spec>& tokenizer,
                                         Func&& f)
            -> parse_result<decltype(std::declval<Func&>()(callback_result_of<Derived>{}))>
        {
            using rule_ = production_rule::detail::make_rule<decltype(Derived::rule())>;
            using rule =
                typename production_rule::detail::eliminate_left_recursion<Derived, rule_>::type;
            using final_parser = production_rule::detail::final_parser<Grammar, Derived>;
            using parser       = production_rule::detail::parser_for<rule, final_parser>;
            return parser::parse(tokenizer, f);
        }

        template <class Func>
        static constexpr auto parse_impl(short, tokenizer<typename Grammar::token_spec>& tokenizer,
                                         Func&& f)
        {
            using rule_ = production_rule::detail::make_rule<decltype(Derived::rule())>;
            using rule =
                typename production_rule::detail::eliminate_left_recursion<Derived, rule_>::type;
            using final_parser = production_rule::detail::final_parser<Grammar, Derived>;
            using parser       = production_rule::detail::parser_for<rule, final_parser>;
            return parser::parse(tokenizer, f);
        }

    public:
        template <class Func>
        static constexpr auto parse(tokenizer<typename Grammar::token_spec>& tokenizer, Func&& f)
            -> decltype(parse_impl(0, tokenizer, f))
        {
            return parse_impl(0, tokenizer, f);
        }
    };
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_RULE_PRODUCTION_HPP_INCLUDED
