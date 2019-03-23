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
            template <class Operator, class OpProduction>
            using parser_for = typename Operator::template parser<OpProduction>;

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

            template <class OpProduction>
            struct parser_impl
            {
                using grammar = typename OpProduction::grammar;
                using tlp     = OpProduction;
                using primary = typename OpProduction::expression::primary;

                template <class Operator, class TokenSpec, class Func>
                static constexpr token<TokenSpec> parse_operator(Operator,
                                                                 tokenizer<TokenSpec>& tokenizer,
                                                                 Func&                 f)
                {
                    auto op = tokenizer.peek();
                    if (!op.is(Operator{}))
                    {
                        auto error = unexpected_token<grammar, tlp, Operator>(tlp{}, Operator{});
                        lex::detail::report_error(f, error, tokenizer);
                        return {};
                    }
                    else
                    {
                        tokenizer.bump();
                        return op;
                    }
                }

                template <class R, class TokenSpec, class Func, class Arg>
                static constexpr R parse_operands(lex::detail::type_list<>, tokenizer<TokenSpec>&,
                                                  Func&, Arg&& lhs)
                {
                    return R::success(static_cast<Arg&&>(lhs));
                }

                template <class R, class Head, class... Tail, class TokenSpec, class Func,
                          class Arg>
                static constexpr R parse_operands(lex::detail::type_list<Head, Tail...>,
                                                  tokenizer<TokenSpec>& tokenizer, Func& f,
                                                  Arg&& lhs)
                {
                    if (peek_token_is(typename Head::leading_tokens{}, tokenizer))
                        return parser_for<Head, OpProduction>::template parse<R>(tokenizer, f,
                                                                                 static_cast<Arg&&>(
                                                                                     lhs));
                    else
                        return parse_operands<R>(lex::detail::type_list<Tail...>{}, tokenizer, f,
                                                 static_cast<Arg&&>(lhs));
                }

                template <class R, class... Operands, class TokenSpec, class Func>
                static constexpr R parse_operands(lex::detail::type_list<Operands...> operands,
                                                  tokenizer<TokenSpec>& tokenizer, Func& f)
                {
                    auto lhs = parser_for<primary, OpProduction>::template parse<R>(tokenizer, f);
                    if (lhs.is_unmatched())
                        return {};
                    else
                        return parse_operands<R>(operands, tokenizer, f,
                                                 lhs.template value_or_tag<tlp>());
                }
            };

            template <class Production>
            struct primary
            {
                template <class OpProduction>
                struct parser
                {
                    template <class R, class TokenSpec, class Func>
                    static constexpr R parse(tokenizer<TokenSpec>& tokenizer, Func& f)
                    {
                        auto operand = Production::parse(tokenizer, f);
                        if (operand.is_unmatched())
                            return {};
                        else
                            return lex::detail::apply_parse_result(f, OpProduction{},
                                                                   operand.template value_or_tag<
                                                                       Production>());
                    }
                };
            };
        } // namespace detail

        template <class Primary, class... LowestOperators>
        struct expression
        {
            static_assert(is_production<Primary>::value, "primary must be a production");

            using primary = detail::primary<Primary>;

            template <class OpProduction>
            struct parser
            {
                using impl = detail::parser_impl<OpProduction>;

                template <class R, class TokenSpec, class Func>
                static constexpr R parse(tokenizer<TokenSpec>& tokenizer, Func& f)
                {
                    return impl::template parse_operands<
                        R>(lex::detail::type_list<LowestOperators...>{}, tokenizer, f);
                }
            };
        };

#if 0 // TODO
        template <class TokenOpen, class TokenClose>
        struct parenthesized
        {
            static_assert(is_token<TokenOpen>::value && is_token<TokenClose>::value,
                          "parentheses must be tokens");

            using leading_token = lex::detail::type_list<TokenOpen>;

            template <class Grammar, class TLP>
            struct parser
            {
                template <class TokenSpec, class Func>
                static constexpr auto parse(tokenizer<TokenSpec>& tokenizer, Func& f)
                    -> decltype(TLP::parse(tokenizer, f))
                {
                    // opening parenthesis
                    auto open = tokenizer.peek();
                    if (!open.is(TokenOpen{}))
                    {
                        auto error = unexpected_token<Grammar, TLP, TokenOpen>(TLP{}, TokenOpen{});
                        lex::detail::report_error(f, error, tokenizer);
                        return {};
                    }
                    else
                        tokenizer.bump();

                    // operand
                    auto operand = TLP::parse(tokenizer, f);
                    if (operand.is_unmatched())
                        return {};

                    // closing parenthesis
                    auto close = tokenizer.peek();
                    if (!open.is(TokenClose{}))
                    {
                        auto error
                            = unexpected_token<Grammar, TLP, TokenClose>(TLP{}, TokenClose{});
                        lex::detail::report_error(f, error, tokenizer);
                        return {};
                    }
                    else
                        tokenizer.bump();

                    return operand;
                }
            };
        };
#endif

        template <class Operator, class... Operands>
        struct bin_op_single
        {
            static_assert(is_token<Operator>::value, "operator must be a token");

            using leading_tokens = lex::detail::concat<lex::detail::type_list<Operator>,
                                                       typename Operands::leading_tokens...>;
            static_assert(lex::detail::is_unique<leading_tokens>::value,
                          "multiple operators use the same leading operand");

            template <class OpProduction>
            struct parser
            {
                using impl = detail::parser_impl<OpProduction>;

                template <class R, class TokenSpec, class Func, class Arg>
                static constexpr R parse_self(tokenizer<TokenSpec>& tokenizer, Func& f, Arg&& lhs)
                {
                    auto op = impl::parse_operator(Operator{}, tokenizer, f);
                    if (!op)
                        return R::success(static_cast<Arg&&>(lhs));

                    auto operand
                        = impl::template parse_operands<R>(lex::detail::type_list<Operands...>{},
                                                           tokenizer, f);
                    if (operand.is_unmatched())
                        return {};

                    return lex::detail::apply_parse_result(f, OpProduction{},
                                                           static_cast<Arg&&>(lhs),
                                                           lex::static_token<Operator>(op),
                                                           operand.template value_or_tag<
                                                               OpProduction>());
                }

                template <class R, class TokenSpec, class Func, class Arg>
                static constexpr R parse(tokenizer<TokenSpec>& tokenizer, Func& f, Arg&& lhs)
                {
                    if (tokenizer.peek().is(Operator{}))
                        return parse_self<R>(tokenizer, f, static_cast<Arg&&>(lhs));
                    else
                    {
                        auto real_lhs = impl::template parse_operands<
                            R>(lex::detail::type_list<Operands...>{}, tokenizer, f,
                               static_cast<Arg&&>(lhs));
                        if (real_lhs.is_unmatched())
                            return {};

                        return parse_self<R>(tokenizer, f,
                                             real_lhs.template value_or_tag<OpProduction>());
                    }
                }
            };
        };
    } // namespace operator_rule

    template <class Derived, class Grammar>
    class operator_production : public detail::base_production
    {
        template <class Func>
        static constexpr auto parse_impl(int, tokenizer<typename Grammar::token_spec>& tokenizer,
                                         Func&& f)
            -> parse_result<decltype(std::declval<Func&>()(callback_result_of<Derived>{}))>
        {
            using return_type
                = parse_result<decltype(std::declval<Func&>()(callback_result_of<Derived>{}))>;
            return operator_rule::detail::parser_for<
                typename Derived::expression, Derived>::template parse<return_type>(tokenizer, f);
        }

        template <class Func>
        static constexpr auto parse_impl(short, tokenizer<typename Grammar::token_spec>&, Func&&)
        {
            return detail::missing_callback_result_of<Func, Derived>{};
        }

    public:
        using grammar = Grammar;

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
