// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_PRODUCTION_RULE_PRODUCTION_HPP_INCLUDED
#define FOONATHAN_LEX_PRODUCTION_RULE_PRODUCTION_HPP_INCLUDED

#include <boost/mp11/set.hpp>

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
                        return lex::detail::missing_callback_result_of<Func, Production>{};
                    }

                    template <class TokenSpec, typename Func, typename... Args>
                    static constexpr auto parse(tokenizer<TokenSpec>& tokenizer, Func& f,
                                                Args&&... args)
                        -> decltype(
                            Cont::parse(tokenizer, f, static_cast<Args&&>(args)...,
                                        callback_return_type(0, f).template forward<Production>()))
                    {
                        auto result = Production::parse(tokenizer, f);
                        if (result.is_success())
                            return Cont::parse(tokenizer, f, static_cast<Args&&>(args)...,
                                               result.template forward<Production>());
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
                            Production::parse(tokenizer, f).template forward<Production>()))
                    {
                        auto result = Production::parse(tokenizer, f);
                        if (result.is_success())
                            return Cont::parse(tokenizer, f, static_cast<Args&&>(args)...,
                                               result.template forward<Production>());
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

            template <class Production>
            struct inline_production : base_rule
            {
                template <class Cont>
                struct parser : Cont
                {
                    static_assert(is_production<Production>::value,
                                  "only a production can be used in this context");

                    template <class TokenSpec, typename Func, typename... Args>
                    struct capture_callback;
                    template <class TokenSpec, typename Func>
                    struct capture_callback<TokenSpec, Func>
                    {
                        tokenizer<TokenSpec>& t;
                        Func&                 f;

                        constexpr capture_callback(tokenizer<TokenSpec>& t, Func& f) : t(t), f(f) {}

                        template <typename... Args>
                        constexpr auto operator()(Args&&... args) const
                        {
                            return Cont::parse(t, f, static_cast<Args&&>(args)...);
                        }
                    };
                    template <class TokenSpec, typename Func, typename Head, typename... Tail>
                    struct capture_callback<TokenSpec, Func, Head, Tail...>
                    : capture_callback<TokenSpec, Func, Tail...>
                    {
                        Head&& head;

                        constexpr capture_callback(tokenizer<TokenSpec>& tokenizer, Func& f,
                                                   Head&& h, Tail&&... tail)
                        : capture_callback<TokenSpec, Func, Tail...>(tokenizer, f,
                                                                     static_cast<Tail&&>(tail)...),
                          head(static_cast<Head&&>(h))
                        {}

                        template <typename... Args>
                        constexpr auto operator()(Args&&... args) const
                        {
                            return capture_callback<TokenSpec, Func, Tail...>::
                                operator()(static_cast<Head&&>(head), static_cast<Args&&>(args)...);
                        }
                    };

                    template <class TokenSpec, typename Func, typename... Args>
                    static constexpr auto parse(tokenizer<TokenSpec>& tokenizer, Func& f,
                                                Args&&... args)
                    {
                        // create a callback that passes the existing arguments as well
                        capture_callback<TokenSpec, Func, Args&&...>
                            impl_callback(tokenizer, f, static_cast<Args&&>(args)...);

                        // parse the production but capture success
                        capture_success_callback<Func, Production, decltype(impl_callback)>
                             callback{f, impl_callback};
                        auto result       = Production::parse(tokenizer, callback);
                        using result_type = std::decay_t<decltype(result)>;
                        if (result.is_success())
                            // we've matched the production, return the parse result from the
                            // continuation
                            return static_cast<result_type&&>(result).value();
                        else
                            // we did not match the production, return an unmatched parse result of
                            // the type of result
                            return typename result_type::value_type{};
                    }
                };
            };

            template <class... Rules>
            struct sequence : base_rule
            {
                static_assert(mp::mp_none_of<sequence, is_choice_rule>::value,
                              "a choice cannot be composed further");

                template <class Cont>
                using parser = parser_for<Rules..., Cont>;
            };

            template <class PeekRule, class Rule>
            struct choice_alternative : base_choice_rule
            {
                using peek_rule = PeekRule;
                using rule      = Rule;

                template <class TokenSpec>
                static constexpr bool peek(tokenizer<TokenSpec> tokenizer)
                {
                    // use alternative if rule matched
                    return is_rule_parsed<PeekRule>(tokenizer);
                }

                template <class Cont>
                using parser = parser_for<Rule, Cont>;
            };

            template <class... Choices>
            struct choice : base_choice_rule
            {
                static_assert(mp::mp_is_set<choice>::value, "duplicate alternatives in a choice");

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
                            auto next_result
                                = try_parse<parser_for<Tail, Cont>>(tokenizer, f,
                                                                    result.template forward<tlp>());
                            if (next_result.is_unmatched())
                                break;
                            result = static_cast<decltype(next_result)&&>(next_result);
                        }

                        return result;
                    }
                };
            };

        } // namespace detail
    }     // namespace production_rule
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_PRODUCTION_RULE_PRODUCTION_HPP_INCLUDED
