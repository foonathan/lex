// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_OPERATOR_PRODUCTION_HPP_INCLUDED
#define FOONATHAN_LEX_OPERATOR_PRODUCTION_HPP_INCLUDED

#include <foonathan/lex/grammar.hpp>
#include <foonathan/lex/parse_error.hpp>
#include <foonathan/lex/parse_result.hpp>
#include <foonathan/lex/tokenizer.hpp>

namespace foonathan
{
namespace lex
{
    namespace operator_rule
    {
        namespace detail
        {
            template <class TLP, class Func>
            using parse_result
                = lex::parse_result<decltype(std::declval<Func>()(callback_result_of<TLP>{}))>;

            template <class... Tokens>
            struct operator_spelling
            {
                template <class TokenSpec>
                static constexpr bool match(const token<TokenSpec>& token)
                {
                    // TODO: C++17
                    return (token.is(Tokens{}) || ...);
                }

                template <class Func, class TLP, class TokenSpec>
                static constexpr parse_result<TLP, Func> apply_prefix(
                    Func& f, TLP, const token<TokenSpec>& op, parse_result<TLP, Func>& value)
                {
                    parse_result<TLP, Func> result;

                    // TODO: C++17
                    (void)((op.is(Tokens{})
                                ? (result = lex::detail::
                                       apply_parse_result(f, TLP{}, lex::static_token<Tokens>(op),
                                                          value.template value_or_tag<TLP>()),
                                   true)
                                : false)
                           || ...);

                    return result;
                }

                template <class Func, class TLP, class TokenSpec>
                static constexpr parse_result<TLP, Func> apply_binary(Func& f, TLP,
                                                                      parse_result<TLP, Func>& lhs,
                                                                      const token<TokenSpec>&  op,
                                                                      parse_result<TLP, Func>& rhs)
                {
                    parse_result<TLP, Func> result;

                    // TODO: C++17
                    (void)((op.is(Tokens{})
                                ? (result = lex::detail::
                                       apply_parse_result(f, TLP{},
                                                          lhs.template value_or_tag<TLP>(),
                                                          lex::static_token<Tokens>(op),
                                                          rhs.template value_or_tag<TLP>()),
                                   true)
                                : false)
                           || ...);

                    return result;
                }
            };

            template <class S1, class S2>
            struct merge_operator_spelling_impl;
            template <class... T1, class... T2>
            struct merge_operator_spelling_impl<operator_spelling<T1...>, operator_spelling<T2...>>
            {
                using type = operator_spelling<T1..., T2...>;
            };
            template <class S1, class S2>
            using merge_operator_spelling = typename merge_operator_spelling_impl<S1, S2>::type;

            template <class Production>
            struct atom
            {
                using binary_ops = operator_spelling<>;

                template <class TokenSpec>
                static constexpr bool has_matching_precedence(const token<TokenSpec>&)
                {
                    return false;
                }

                template <class TLP, class TokenSpec, class Func>
                static constexpr auto parse_infix_operand(tokenizer<TokenSpec>& tokenizer, Func& f)
                    -> parse_result<TLP, Func>
                {
                    auto value = Production::parse(tokenizer, f);
                    if (value.is_unmatched())
                        return {};
                    else
                        return lex::detail::apply_parse_result(f, TLP{},
                                                               value.template value_or_tag<
                                                                   Production>());
                }

                template <class TLP, class TokenSpec, class Func>
                static constexpr auto parse_binary(tokenizer<TokenSpec>& tokenizer, Func& f)
                    -> parse_result<TLP, Func>
                {
                    return parse_infix_operand<TLP>(tokenizer, f);
                }
            };

            template <class Operator, class Operand>
            struct prefix_op_single
            {
                using binary_ops = typename Operand::binary_ops;

                template <class TokenSpec>
                static constexpr bool has_matching_precedence(const token<TokenSpec>& op)
                {
                    return Operand::has_matching_precedence(op);
                }

                template <class TLP, class TokenSpec, class Func>
                static constexpr auto parse_infix_operand(tokenizer<TokenSpec>& tokenizer, Func& f)
                    -> parse_result<TLP, Func>
                {
                    auto op = tokenizer.peek();
                    if (Operator::match(op))
                    {
                        tokenizer.bump();

                        auto operand = Operand::template parse_infix_operand<TLP>(tokenizer, f);
                        if (operand.is_unmatched())
                            return {};

                        return Operator::apply_prefix(f, TLP{}, op, operand);
                    }
                    else
                        return Operand::template parse_infix_operand<TLP>(tokenizer, f);
                }

                template <class TLP, class TokenSpec, class Func>
                static constexpr auto parse_binary(tokenizer<TokenSpec>& tokenizer, Func& f)
                    -> parse_result<TLP, Func>
                {
                    return parse_infix_operand<TLP>(tokenizer, f);
                }
            };

            template <class Operator, class Operand>
            struct binary_op_single
            {
                using binary_ops = merge_operator_spelling<Operator, typename Operand::binary_ops>;

                template <class TokenSpec>
                static constexpr bool has_matching_precedence(const token<TokenSpec>& op)
                {
                    return Operator::match(op) || Operand::has_matching_precedence(op);
                }

                template <class TLP, class TokenSpec, class Func>
                static constexpr auto parse_infix_operand(tokenizer<TokenSpec>& tokenizer, Func& f)
                    -> parse_result<TLP, Func>
                {
                    return Operand::template parse_infix_operand<TLP>(tokenizer, f);
                }

                template <class TLP, class TokenSpec, class Func>
                static constexpr auto parse_binary(tokenizer<TokenSpec>& tokenizer, Func& f)
                    -> parse_result<TLP, Func>
                {
                    auto result = parse_infix_operand<TLP>(tokenizer, f);
                    if (result.is_unmatched())
                        return {};

                    while (true)
                    {
                        auto op = tokenizer.peek();
                        if (!has_matching_precedence(op))
                            break;
                        else
                            tokenizer.bump();

                        auto operand = Operand::template parse_binary<TLP>(tokenizer, f);
                        if (operand.is_unmatched())
                            return {};

                        result = binary_ops::apply_binary(f, TLP{}, result, op, operand);
                        if (Operator::match(op))
                            break;
                    }

                    return result;
                }
            };

            template <class Child>
            struct expression
            {
                template <class TLP, class TokenSpec, class Func>
                static constexpr auto parse(tokenizer<TokenSpec>& tokenizer, Func& f)
                    -> parse_result<TLP, Func>
                {
                    auto result = Child::template parse_binary<TLP>(tokenizer, f);
                    if (result.is_unmatched())
                        return {};
                    else if (Child::has_matching_precedence(tokenizer.peek()))
                        // TODO: error
                        return {};
                    else
                        return result;
                }
            };

        } // namespace detail

        template <class Production>
        constexpr auto atom = detail::atom<Production>{};

        template <class Token, class Operand>
        constexpr auto pre_op_single(Token, Operand)
        {
            return detail::prefix_op_single<detail::operator_spelling<Token>, Operand>{};
        }

        template <class Token, class Operand>
        constexpr auto bin_op_single(Token, Operand)
        {
            return detail::binary_op_single<detail::operator_spelling<Token>, Operand>{};
        }

        template <class Operator>
        constexpr auto expression(Operator)
        {
            return detail::expression<Operator>{};
        }
    } // namespace operator_rule

    template <class Derived, class Grammar>
    class operator_production : public detail::base_production
    {
        template <class Func>
        static constexpr auto parse_impl(int, tokenizer<typename Grammar::token_spec>& tokenizer,
                                         Func&& f)
            -> operator_rule::detail::parse_result<Derived, Func>
        {
            using expr = decltype(Derived::expression());
            return expr::template parse<Derived>(tokenizer, f);
        }

        template <class Func>
        static constexpr auto parse_impl(short, tokenizer<typename Grammar::token_spec>&, Func&&)
        {
            return detail::missing_callback_result_of<Func, Derived>{};
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

#endif // FOONATHAN_LEX_OPERATOR_PRODUCTION_HPP_INCLUDED
