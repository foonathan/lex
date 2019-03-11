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

            template <class Rule>
            std::true_type check_peekable(int, typename Rule::peek_tokens = {});
            template <class Rule>
            std::false_type check_peekable(short);

            template <typename T>
            struct is_peekable_rule : decltype(check_peekable<T>(0))
            {};

            //=== rules ===//
            template <class Production>
            struct production : base_rule
            {
                static_assert(is_production<Production>::value,
                              "only a production can be used in this context");

                template <class Cont>
                struct parser : Cont
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
            };

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

            template <class... Rules>
            struct sequence : base_rule
            {
                static_assert(
                    lex::detail::none_of<lex::detail::type_list<Rules...>, is_choice_rule>::value,
                    "a choice cannot be composed further");

                template <class Cont>
                using parser = parser_for<Rules..., Cont>;
            };

            template <class TokenRule, class Rule>
            struct choice_alternative : base_rule
            {
                using peek_tokens = typename TokenRule::peek_tokens;

                template <class TokenSpec>
                static constexpr bool peek(tokenizer<TokenSpec> tokenizer)
                {
                    // use alternative if rule matched
                    ignore_callback f;
                    return parser_for<TokenRule, test_parser<TokenSpec>>::parse(tokenizer, f)
                        .is_success();
                }

                template <class Cont>
                using parser = parser_for<Rule, Cont>;
            };

            template <class... Choices>
            struct choice : base_choice_rule
            {
                using peek_tokens = lex::detail::concat<lex::detail::type_list<>,
                                                        typename Choices::peek_tokens...>;
                static_assert(lex::detail::is_unique<peek_tokens>::value,
                              "choice cannot be resolved with one token lookahead");

                template <class Cont>
                struct parser : Cont
                {
                    using grammar = typename Cont::grammar;
                    using tlp     = typename Cont::tlp;

                    template <class... Tokens, class TokenSpec, typename Func>
                    static constexpr void report_error(lex::detail::type_list<Tokens...>,
                                                       tokenizer<TokenSpec>& tokenizer, Func& f)
                    {
                        token_kind<TokenSpec> alternatives[] = {Tokens{}...};
                        auto                  error
                            = exhausted_token_choice<grammar, tlp, Tokens...>(tlp{}, alternatives);
                        lex::detail::report_error(f, error, tokenizer);
                    }

                    template <class R, class TokenSpec, typename Func, typename... Args>
                    static constexpr R parse_impl(choice<>, tokenizer<TokenSpec>& tokenizer,
                                                  Func& f, Args&&...)
                    {
                        using all_tokens = lex::detail::concat<lex::detail::type_list<>,
                                                               typename Choices::peek_tokens...>;
                        report_error(all_tokens{}, tokenizer, f);
                        return {};
                    }
                    template <class R, class Head, class... Tail, class TokenSpec, typename Func,
                              typename... Args>
                    static constexpr R parse_impl(choice<Head, Tail...>,
                                                  tokenizer<TokenSpec>& tokenizer, Func& f,
                                                  Args&&... args)
                    {
                        if (Head::peek(tokenizer))
                            return parser_for<Head, Cont>::parse(tokenizer, f,
                                                                 static_cast<Args&&>(args)...);
                        else
                            return parse_impl<R>(choice<Tail...>{}, tokenizer, f,
                                                 static_cast<Args&&>(args)...);
                    }

                    template <class TokenSpec, typename Func, typename... Args>
                    static constexpr auto parse(tokenizer<TokenSpec>& tokenizer, Func& f,
                                                Args&&... args)
                    {
                        using return_type = std::common_type_t<decltype(
                            parser_for<Choices, Cont>::parse(tokenizer, f,
                                                             static_cast<Args&&>(args)...))...>;
                        return parse_impl<return_type>(choice<Choices...>{}, tokenizer, f,
                                                       static_cast<Args&&>(args)...);
                    }
                };
            };
        } // namespace detail
    }     // namespace production_rule
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_PRODUCTION_RULE_PRODUCTION_HPP_INCLUDED
