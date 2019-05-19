// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_OPERATOR_PRODUCTION_HPP_INCLUDED
#define FOONATHAN_LEX_OPERATOR_PRODUCTION_HPP_INCLUDED

#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/list.hpp>
#include <boost/mp11/set.hpp>

#include <foonathan/lex/grammar.hpp>
#include <foonathan/lex/parser.hpp>
#include <foonathan/lex/tokenizer.hpp>

namespace foonathan
{
namespace lex
{
    namespace operator_rule
    {
        /// \exclude
        struct operator_adl
        {};

        namespace detail
        {
            namespace mp = boost::mp11;

            template <class TLP, class Func>
            struct op_parse_result
            {
                using value_type = decltype(std::declval<Func>()(callback_result_of<TLP>{}));

                parse_result<value_type>                 result;
                token<typename TLP::grammar::token_spec> op;

                constexpr bool is_unmatched() const noexcept
                {
                    return result.is_unmatched();
                }

                constexpr value_type&& forward() noexcept
                {
                    return result.template forward<TLP>();
                }
            };

            //=== operator_spelling ===//
            template <class... Tokens>
            struct operator_spelling
            {
                static_assert(mp::mp_all_of<operator_spelling, is_token>::value,
                              "operators must be single tokens");

                using token_list = operator_spelling;

                template <class TokenSpec>
                static constexpr bool match(const token<TokenSpec>& token)
                {
                    bool result[] = {token.is(Tokens{})..., false};
                    for (auto value : result)
                        if (value)
                            return true;
                    return false;
                }

                template <class Func, class TLP, class TokenSpec>
                static constexpr op_parse_result<TLP, Func> apply_prefix(
                    Func& f, TLP, const token<TokenSpec>& op, op_parse_result<TLP, Func>& value)
                {
                    op_parse_result<TLP, Func> result;
                    result.op = op;

                    bool dummy[]
                        = {(op.is(Tokens{})
                            && (result.result
                                = lex::detail::apply_parse_result(f, TLP{},
                                                                  lex::static_token<Tokens>(op),
                                                                  value.forward()),
                                false))...,
                           false};
                    (void)dummy;

                    return result;
                }

                template <class Func, class TLP, class TokenSpec>
                static constexpr op_parse_result<TLP, Func> apply_postfix(
                    Func& f, TLP, op_parse_result<TLP, Func>& value, const token<TokenSpec>& op)
                {
                    op_parse_result<TLP, Func> result;
                    result.op = op;

                    bool dummy[]
                        = {(op.is(Tokens{})
                            && (result.result
                                = lex::detail::apply_parse_result(f, TLP{}, value.forward(),
                                                                  lex::static_token<Tokens>(op)),
                                false))...,
                           false};
                    (void)dummy;

                    return result;
                }

                template <class Func, class TLP, class TokenSpec>
                static constexpr op_parse_result<TLP, Func> apply_binary(
                    Func& f, TLP, op_parse_result<TLP, Func>& lhs, const token<TokenSpec>& op,
                    op_parse_result<TLP, Func>& rhs)
                {
                    op_parse_result<TLP, Func> result;
                    result.op = op;

                    bool dummy[]
                        = {(op.is(Tokens{})
                            && (result.result
                                = lex::detail::apply_parse_result(f, TLP{}, lhs.forward(),
                                                                  lex::static_token<Tokens>(op),
                                                                  rhs.forward()),
                                false))...,
                           false};
                    (void)dummy;

                    return result;
                }
            };

            //=== operator parsers ===//
            template <class Parser, class TLP, class TokenSpec, class Func>
            constexpr auto parse(tokenizer<TokenSpec>& tokenizer, Func& f)
                -> op_parse_result<TLP, Func>
            {
                auto lhs = Parser::template parse_null<TLP>(tokenizer, f);
                if (lhs.is_unmatched())
                    return lhs;

                return Parser::template parse_left<TLP>(tokenizer, f, lhs);
            }

            struct operand_parser : operator_adl
            {};

            template <class Operand>
            constexpr void verify_operand()
            {
                constexpr bool is_operator_dsl = std::is_base_of<operator_adl, Operand>::value;
                constexpr bool is_operand      = std::is_base_of<operand_parser, Operand>::value;

                static_assert(is_operator_dsl, "invalid type in operator rule DSL");
                static_assert(is_operand, "cannot nest this operator rule further");
            }

            template <class ProductionOrToken>
            struct atom : operand_parser
            {
                static_assert(is_production<ProductionOrToken>::value
                                  || is_token<ProductionOrToken>::value,
                              "atom must be a production or token");

                using pre_tokens  = mp::mp_list<>;
                using post_tokens = mp::mp_list<>;

                template <class TLP, class TokenSpec, class Func>
                static constexpr auto parse_impl(std::true_type /* is_production */,
                                                 tokenizer<TokenSpec>& tokenizer, Func&& f)
                {
                    return ProductionOrToken::parse(tokenizer, f);
                }
                template <class TLP, class TokenSpec, class Func>
                static constexpr auto parse_impl(std::false_type /* is_production */,
                                                 tokenizer<TokenSpec>& tokenizer, Func&& f)
                {
                    auto token        = tokenizer.peek();
                    using result_type = lex::parse_result<decltype(
                        lex::detail::parse_token<ProductionOrToken>(token))>;
                    if (token.is(ProductionOrToken{}))
                    {
                        tokenizer.bump();
                        return result_type::success(
                            lex::detail::parse_token<ProductionOrToken>(token));
                    }
                    else
                    {
                        auto error
                            = unexpected_token<typename TLP::grammar, TLP,
                                               ProductionOrToken>(TLP{}, ProductionOrToken{});
                        lex::detail::report_error(f, error, tokenizer);
                        return result_type::unmatched();
                    }
                }

                template <class TLP, class TokenSpec, class Func>
                static constexpr auto parse_null(tokenizer<TokenSpec>& tokenizer, Func& f)
                    -> op_parse_result<TLP, Func>
                {
                    auto value = parse_impl<TLP>(is_production<ProductionOrToken>{}, tokenizer, f);
                    if (value.is_unmatched())
                        return {};
                    else
                        return {lex::detail::apply_parse_result(f, TLP{},
                                                                value.template forward<
                                                                    ProductionOrToken>()),
                                {}};
                }

                template <class TLP, class TokenSpec, class Func>
                static constexpr auto parse_left(tokenizer<TokenSpec>&, Func&,
                                                 op_parse_result<TLP, Func>& lhs)
                {
                    return static_cast<op_parse_result<TLP, Func>&&>(lhs);
                }
            };

            template <class TokenOpen, class TokenClose, class Atom>
            struct parenthesized : operand_parser
            {
                static_assert(is_token<TokenOpen>::value && is_token<TokenClose>::value,
                              "parentheses must be tokens");

                using pre_tokens  = mp::mp_list<>;
                using post_tokens = mp::mp_list<>;

                template <class TLP, class TokenSpec, class Func>
                static constexpr auto parse_null(tokenizer<TokenSpec>& tokenizer, Func& f)
                    -> op_parse_result<TLP, Func>
                {
                    if (tokenizer.peek().is(TokenOpen{}))
                    {
                        tokenizer.bump();

                        auto value = TLP::parse(tokenizer, f);
                        if (value.is_unmatched())
                            return {};

                        if (tokenizer.peek().is(TokenClose{}))
                            tokenizer.bump();
                        else
                        {
                            auto error = unexpected_token<typename TLP::grammar, TLP,
                                                          TokenClose>(TLP{}, TokenClose{});
                            lex::detail::report_error(f, error, tokenizer);
                            return {};
                        }

                        return {static_cast<decltype(value)&&>(value), {}};
                    }
                    else
                        return Atom::template parse_null<TLP>(tokenizer, f);
                }

                template <class TLP, class TokenSpec, class Func>
                static constexpr auto parse_left(tokenizer<TokenSpec>&, Func&,
                                                 op_parse_result<TLP, Func>& lhs)
                {
                    return static_cast<op_parse_result<TLP, Func>&&>(lhs);
                }
            };

            enum associativity
            {
                single,
                left,
                right,
            };

            template <associativity Assoc, class Operator, class Operand>
            struct prefix_op : operand_parser
            {
                using pre_tokens
                    = mp::mp_append<typename Operator::token_list, typename Operand::pre_tokens>;
                using post_tokens = typename Operand::post_tokens;

                template <class TLP, class TokenSpec, class Func>
                static constexpr auto parse_null(tokenizer<TokenSpec>& tokenizer, Func& f)
                    -> op_parse_result<TLP, Func>
                {
                    auto op = tokenizer.peek();
                    if (Operator::match(op))
                    {
                        tokenizer.bump();

                        auto operand = Assoc == single ? parse<Operand, TLP>(tokenizer, f)
                                                       : parse<prefix_op, TLP>(tokenizer, f);
                        if (operand.is_unmatched())
                            return operand;

                        return Operator::apply_prefix(f, TLP{}, op, operand);
                    }
                    else
                        return Operand::template parse_null<TLP>(tokenizer, f);
                }

                template <class TLP, class TokenSpec, class Func>
                static constexpr auto parse_left(tokenizer<TokenSpec>& tokenizer, Func& f,
                                                 op_parse_result<TLP, Func>& lhs)
                    -> op_parse_result<TLP, Func>
                {
                    return Operand::template parse_left<TLP>(tokenizer, f, lhs);
                }
            };

            template <associativity Assoc, class Operator, class Production, class Operand>
            struct prefix_prod : operand_parser
            {
                static_assert(is_production<Production>::value, "must be a production");

                using pre_tokens
                    = mp::mp_append<typename Operator::token_list, typename Operand::pre_tokens>;
                using post_tokens = typename Operand::post_tokens;

                template <class TLP, class TokenSpec, class Func>
                static constexpr auto parse_null(tokenizer<TokenSpec>& tokenizer, Func& f)
                    -> op_parse_result<TLP, Func>
                {
                    auto op_token = tokenizer.peek();
                    if (Operator::match(op_token))
                    {
                        auto op = Production::parse(tokenizer, f);
                        if (op.is_unmatched())
                            return {};

                        auto operand = Assoc == single ? parse<Operand, TLP>(tokenizer, f)
                                                       : parse<prefix_prod, TLP>(tokenizer, f);
                        if (operand.is_unmatched())
                            return operand;

                        return {lex::detail::apply_parse_result(f, TLP{},
                                                                op.template forward<Production>(),
                                                                operand.forward()),
                                op_token};
                    }
                    else
                        return Operand::template parse_null<TLP>(tokenizer, f);
                }

                template <class TLP, class TokenSpec, class Func>
                static constexpr auto parse_left(tokenizer<TokenSpec>& tokenizer, Func& f,
                                                 op_parse_result<TLP, Func>& lhs)
                    -> op_parse_result<TLP, Func>
                {
                    return Operand::template parse_left<TLP>(tokenizer, f, lhs);
                }
            };

            template <associativity Assoc, class Operator, class Operand>
            struct postfix_op : operand_parser
            {
                using pre_tokens = typename Operand::pre_tokens;
                using post_tokens
                    = mp::mp_append<typename Operator::token_list, typename Operand::post_tokens>;

                template <class TLP, class TokenSpec, class Func>
                static constexpr auto parse_null(tokenizer<TokenSpec>& tokenizer, Func& f)
                    -> op_parse_result<TLP, Func>
                {
                    return Operand::template parse_null<TLP>(tokenizer, f);
                }

                template <class TLP, class TokenSpec, class Func>
                static constexpr auto parse_left(tokenizer<TokenSpec>& tokenizer, Func& f,
                                                 op_parse_result<TLP, Func>& lhs)
                    -> op_parse_result<TLP, Func>
                {
                    lhs = Operand::template parse_left<TLP>(tokenizer, f, lhs);
                    if (lhs.is_unmatched())
                        return static_cast<op_parse_result<TLP, Func>&&>(lhs);

                    while (Operator::match(tokenizer.peek()))
                    {
                        auto op = tokenizer.get();
                        lhs     = Operator::apply_postfix(f, TLP{}, lhs, op);

                        if (Assoc == single)
                            break;
                    }

                    return static_cast<op_parse_result<TLP, Func>&&>(lhs);
                }
            };

            template <associativity Assoc, class Operator, class Production, class Operand>
            struct postfix_prod : operand_parser
            {
                static_assert(is_production<Production>::value, "must be a production");

                using pre_tokens = typename Operand::pre_tokens;
                using post_tokens
                    = mp::mp_append<typename Operator::token_list, typename Operand::post_tokens>;

                template <class TLP, class TokenSpec, class Func>
                static constexpr auto parse_null(tokenizer<TokenSpec>& tokenizer, Func& f)
                    -> op_parse_result<TLP, Func>
                {
                    return Operand::template parse_null<TLP>(tokenizer, f);
                }

                template <class TLP, class TokenSpec, class Func>
                static constexpr auto parse_left(tokenizer<TokenSpec>& tokenizer, Func& f,
                                                 op_parse_result<TLP, Func>& lhs)
                    -> op_parse_result<TLP, Func>
                {
                    lhs = Operand::template parse_left<TLP>(tokenizer, f, lhs);
                    if (lhs.is_unmatched())
                        return static_cast<op_parse_result<TLP, Func>&&>(lhs);

                    while (Operator::match(tokenizer.peek()))
                    {
                        auto op_token = tokenizer.peek();
                        auto op       = Production::parse(tokenizer, f);
                        if (op.is_unmatched())
                            return {};

                        lhs = {lex::detail::apply_parse_result(f, TLP{}, lhs.forward(),
                                                               op.template forward<Production>()),
                               op_token};
                        if (Assoc == single)
                            break;
                    }

                    return static_cast<op_parse_result<TLP, Func>&&>(lhs);
                }
            };

            template <associativity Assoc, class Operator, class Operand>
            struct binary_op : operand_parser
            {
                using pre_tokens = typename Operand::pre_tokens;
                using post_tokens
                    = mp::mp_append<typename Operator::token_list, typename Operand::post_tokens>;

                template <class TLP, class TokenSpec, class Func>
                static constexpr auto parse_null(tokenizer<TokenSpec>& tokenizer, Func& f)
                    -> op_parse_result<TLP, Func>
                {
                    return Operand::template parse_null<TLP>(tokenizer, f);
                }

                template <class TLP, class TokenSpec, class Func>
                static constexpr auto parse_left(tokenizer<TokenSpec>& tokenizer, Func& f,
                                                 op_parse_result<TLP, Func>& lhs)
                    -> op_parse_result<TLP, Func>
                {
                    lhs = Operand::template parse_left<TLP>(tokenizer, f, lhs);
                    if (lhs.is_unmatched())
                        return static_cast<op_parse_result<TLP, Func>&&>(lhs);

                    while (Operator::match(tokenizer.peek()))
                    {
                        auto op = tokenizer.get();

                        auto operand = Assoc == right ? parse<binary_op, TLP>(tokenizer, f)
                                                      : parse<Operand, TLP>(tokenizer, f);
                        if (operand.is_unmatched())
                            return static_cast<decltype(operand)&&>(operand);

                        lhs = Operator::apply_binary(f, TLP{}, lhs, op, operand);
                        if (Assoc == single && Operator::match(op))
                            break;
                    }

                    return static_cast<op_parse_result<TLP, Func>&&>(lhs);
                }
            };

            template <associativity Assoc, class Operator, class Production, class Operand>
            struct binary_prod : operand_parser
            {
                static_assert(is_production<Production>::value, "must be a production");

                using pre_tokens = typename Operand::pre_tokens;
                using post_tokens
                    = mp::mp_append<typename Operator::token_list, typename Operand::post_tokens>;

                template <class TLP, class TokenSpec, class Func>
                static constexpr auto parse_null(tokenizer<TokenSpec>& tokenizer, Func& f)
                    -> op_parse_result<TLP, Func>
                {
                    return Operand::template parse_null<TLP>(tokenizer, f);
                }

                template <class TLP, class TokenSpec, class Func>
                static constexpr auto parse_left(tokenizer<TokenSpec>& tokenizer, Func& f,
                                                 op_parse_result<TLP, Func>& lhs)
                    -> op_parse_result<TLP, Func>
                {
                    lhs = Operand::template parse_left<TLP>(tokenizer, f, lhs);
                    if (lhs.is_unmatched())
                        return static_cast<op_parse_result<TLP, Func>&&>(lhs);

                    while (Operator::match(tokenizer.peek()))
                    {
                        auto op_token = tokenizer.peek();
                        auto op       = Production::parse(tokenizer, f);
                        if (op.is_unmatched())
                            return {};

                        auto operand = Assoc == right ? parse<binary_prod, TLP>(tokenizer, f)
                                                      : parse<Operand, TLP>(tokenizer, f);
                        if (operand.is_unmatched())
                            return operand;

                        lhs = {lex::detail::apply_parse_result(f, TLP{}, lhs.forward(),
                                                               op.template forward<Production>(),
                                                               operand.forward()),
                               op_token};
                        if (Assoc == single && Operator::match(op_token))
                            break;
                    }

                    return static_cast<op_parse_result<TLP, Func>&&>(lhs);
                }
            };

            template <class... Children>
            struct rule : operator_adl
            {
                using pre_tokens  = mp::mp_append<typename Children::pre_tokens...>;
                using post_tokens = mp::mp_append<typename Children::post_tokens...>;

                static_assert(
                    mp::mp_is_set<pre_tokens>::value,
                    "operator choice cannot uniquely decide a path based on a prefix operator, "
                    "try extracting common operands as a separate production");

                template <class TLP>
                struct parse_left_impl
                {
                    template <class... List>
                    struct fn // empty
                    {
                        template <class TokenSpec, class Func>
                        static constexpr auto parse(tokenizer<TokenSpec>&, Func&,
                                                    op_parse_result<TLP, Func>& lhs)
                            -> op_parse_result<TLP, Func>
                        {
                            // no left operator found
                            return static_cast<op_parse_result<TLP, Func>&&>(lhs);
                        }
                    };
                    template <class Head, class... Tail>
                    struct fn<Head, Tail...> // non-empty
                    {
                        template <class TokenSpec, class Func>
                        static constexpr auto parse(tokenizer<TokenSpec>& tokenizer, Func& f,
                                                    op_parse_result<TLP, Func>& lhs)
                            -> op_parse_result<TLP, Func>
                        {
                            auto old    = tokenizer.current_ptr();
                            auto result = Head::template parse_left<TLP>(tokenizer, f, lhs);
                            if (tokenizer.current_ptr() != old)
                                // Head parsed something, so we're done
                                return result;
                            else
                                // try the next child
                                return fn<Tail...>::parse(tokenizer, f, lhs);
                        }
                    };
                };

                template <class TLP>
                struct parse_impl
                {
                    template <class... List>
                    struct fn // empty
                    {
                        template <class TokenSpec, class Func>
                        static constexpr auto parse(tokenizer<TokenSpec>& tokenizer, Func& f)
                            -> op_parse_result<TLP, Func>
                        {
                            // we haven't found any prefix operator, so parse an atom
                            // then use the first child whose parse_left() function matched
                            // something

                            // to parse an atom, we can just use the parse_null() function of any
                            // child, as we didn't have a prefix operator that would match
                            auto atom = mp::mp_front<rule>::template parse_null<TLP>(tokenizer, f);
                            return mp::mp_apply_q<parse_left_impl<TLP>, rule>::parse(tokenizer, f,
                                                                                     atom);
                        }
                    };
                    template <class Head, class... Tail>
                    struct fn<Head, Tail...> // non-empty
                    {
                        template <class TokenSpec, class Func>
                        static constexpr auto parse(tokenizer<TokenSpec>& tokenizer, Func& f)
                            -> op_parse_result<TLP, Func>
                        {
                            if (mp::mp_rename<typename Head::pre_tokens, operator_spelling>::match(
                                    tokenizer.peek()))
                                // we have a prefix operator that definitely narrows it down to Head
                                return detail::parse<Head, TLP>(tokenizer, f);
                            else
                                // try to parse the next one instead
                                return fn<Tail...>::parse(tokenizer, f);
                        }
                    };
                };

                template <class TLP, class TokenSpec, class Func>
                static constexpr auto parse(tokenizer<TokenSpec>& tokenizer, Func& f)
                {
                    return mp::mp_apply_q<parse_impl<TLP>, rule>::parse(tokenizer, f);
                }
            };

            template <class Rule>
            struct end : operator_adl
            {
                template <class TLP, class TokenSpec, class Func>
                static constexpr auto parse(tokenizer<TokenSpec>& tokenizer, Func& f)
                    -> op_parse_result<TLP, Func>
                {
                    auto result = Rule::template parse<TLP>(tokenizer, f);
                    if (result.is_unmatched())
                        return result;
                    else if (mp::mp_rename<typename Rule::post_tokens, operator_spelling>::match(
                                 tokenizer.peek()))
                    {
                        auto error
                            = illegal_operator_chain<typename TLP::grammar, TLP>(TLP{}, result.op);
                        lex::detail::report_error(f, error, tokenizer);
                        return {};
                    }
                    else
                        return result;
                }
            };

            template <class Child>
            struct make_rule_impl
            {
                using type = rule<Child>;
            };
            template <class... Children>
            struct make_rule_impl<rule<Children...>>
            {
                using type = rule<Children...>;
            };
            template <class Rule>
            struct make_rule_impl<end<Rule>>
            {
                using type = end<Rule>;
            };
            template <class Rule>
            using make_rule = typename make_rule_impl<Rule>::type;

            template <class Operand1, class Operand2>
            struct make_choice_impl
            {
                static constexpr auto verify1 = &detail::verify_operand<Operand1>;
                static constexpr auto verify2 = &detail::verify_operand<Operand2>;
                using type                    = rule<Operand1, Operand2>;
            };
            template <class... Children, class Operand2>
            struct make_choice_impl<rule<Children...>, Operand2>
            {
                static constexpr auto verify = &detail::verify_operand<Operand2>;
                using type                   = rule<Children..., Operand2>;
            };
            template <class Operand1, class Operand2>
            using make_choice = typename make_choice_impl<Operand1, Operand2>::type;
        } // namespace detail

        template <class ProductionOrToken>
        constexpr auto atom = detail::atom<ProductionOrToken>{};

        /// \exclude
        template <class TokenOpen, class TokenClose>
        struct parenthesized_t
        {};

        template <class TokenOpen, class TokenClose>
        constexpr parenthesized_t<TokenOpen, TokenClose> parenthesized{};

        template <class Production, class TokenOpen, class TokenClose>
        constexpr auto operator/(detail::atom<Production>, parenthesized_t<TokenOpen, TokenClose>)
        {
            return detail::parenthesized<TokenOpen, TokenClose, detail::atom<Production>>{};
        }

        template <class... Operator, class Operand>
        constexpr auto pre_op_single(Operand)
        {
            detail::verify_operand<Operand>();
            return detail::prefix_op<detail::single, detail::operator_spelling<Operator...>,
                                     Operand>{};
        }
        template <class... Operator, class Operand>
        constexpr auto pre_op_chain(Operand)
        {
            detail::verify_operand<Operand>();
            return detail::prefix_op<detail::left, detail::operator_spelling<Operator...>,
                                     Operand>{};
        }

        template <class... Operator, class Production, class Operand>
        constexpr auto pre_prod_single(Production, Operand)
        {
            detail::verify_operand<Operand>();
            return detail::prefix_prod<detail::single, detail::operator_spelling<Operator...>,
                                       Production, Operand>{};
        }
        template <class... Operator, class Production, class Operand>
        constexpr auto pre_prod_chain(Production, Operand)
        {
            detail::verify_operand<Operand>();
            return detail::prefix_prod<detail::left, detail::operator_spelling<Operator...>,
                                       Production, Operand>{};
        }

        template <class... Operator, class Operand>
        constexpr auto post_op_single(Operand)
        {
            detail::verify_operand<Operand>();
            return detail::postfix_op<detail::single, detail::operator_spelling<Operator...>,
                                      Operand>{};
        }
        template <class... Operator, class Operand>
        constexpr auto post_op_chain(Operand)
        {
            detail::verify_operand<Operand>();
            return detail::postfix_op<detail::right, detail::operator_spelling<Operator...>,
                                      Operand>{};
        }

        template <class... Operator, class Production, class Operand>
        constexpr auto post_prod_single(Production, Operand)
        {
            detail::verify_operand<Operand>();
            return detail::postfix_prod<detail::single, detail::operator_spelling<Operator...>,
                                        Production, Operand>{};
        }
        template <class... Operator, class Production, class Operand>
        constexpr auto post_prod_chain(Production, Operand)
        {
            detail::verify_operand<Operand>();
            return detail::postfix_prod<detail::right, detail::operator_spelling<Operator...>,
                                        Production, Operand>{};
        }

        template <class... Operator, class Operand>
        constexpr auto bin_op_single(Operand)
        {
            detail::verify_operand<Operand>();
            return detail::binary_op<detail::single, detail::operator_spelling<Operator...>,
                                     Operand>{};
        }
        template <class... Operator, class Operand>
        constexpr auto bin_op_left(Operand)
        {
            detail::verify_operand<Operand>();
            return detail::binary_op<detail::left, detail::operator_spelling<Operator...>,
                                     Operand>{};
        }
        template <class... Operator, class Operand>
        constexpr auto bin_op_right(Operand)
        {
            detail::verify_operand<Operand>();
            return detail::binary_op<detail::right, detail::operator_spelling<Operator...>,
                                     Operand>{};
        }

        template <class... Operator, class Production, class Operand>
        constexpr auto bin_prod_single(Production, Operand)
        {
            detail::verify_operand<Operand>();
            return detail::binary_prod<detail::single, detail::operator_spelling<Operator...>,
                                       Production, Operand>{};
        }
        template <class... Operator, class Production, class Operand>
        constexpr auto bin_prod_left(Production, Operand)
        {
            detail::verify_operand<Operand>();
            return detail::binary_prod<detail::left, detail::operator_spelling<Operator...>,
                                       Production, Operand>{};
        }
        template <class... Operator, class Production, class Operand>
        constexpr auto bin_prod_right(Production, Operand)
        {
            detail::verify_operand<Operand>();
            return detail::binary_prod<detail::right, detail::operator_spelling<Operator...>,
                                       Production, Operand>{};
        }

        template <class Operand1, class Operand2,
                  typename = std::enable_if_t<std::is_base_of<operator_adl, Operand1>::value>>
        constexpr auto operator/(Operand1, Operand2)
        {
            return detail::make_choice<Operand1, Operand2>{};
        }

        /// \exclude
        struct end_t
        {};

        constexpr end_t end = {};

        template <class Operand>
        constexpr auto operator+(Operand, end_t)
        {
            static_assert(std::is_base_of<operator_adl, Operand>::value,
                          "illegal type in operator DSL");
            return detail::end<detail::make_rule<Operand>>{};
        }
    } // namespace operator_rule

    template <class Derived, class Grammar>
    class operator_production : public detail::base_production
    {
        template <class Func>
        static constexpr auto parse_impl(int, tokenizer<typename Grammar::token_spec>& tokenizer,
                                         Func&& f)
            -> parse_result<
                typename operator_rule::detail::op_parse_result<Derived, Func>::value_type>
        {
            using rule = operator_rule::detail::make_rule<decltype(Derived::rule())>;
            return rule::template parse<Derived>(tokenizer, f).result;
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
