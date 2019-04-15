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
        /// \exclude
        struct operator_adl
        {};

        namespace detail
        {
            template <class TLP, class Func>
            using parse_result
                = lex::parse_result<decltype(std::declval<Func>()(callback_result_of<TLP>{}))>;

            //=== operator_spelling ===//
            template <class... Tokens>
            struct operator_spelling
            {
                static_assert(
                    lex::detail::all_of<lex::detail::type_list<Tokens...>, is_token>::value,
                    "operators must be single tokens");

                using token_list = lex::detail::type_list<Tokens...>;

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
                static constexpr parse_result<TLP, Func> apply_postfix(
                    Func& f, TLP, parse_result<TLP, Func>& value, const token<TokenSpec>& op)
                {
                    parse_result<TLP, Func> result;

                    // TODO: C++17
                    (void)((op.is(Tokens{})
                                ? (result
                                   = lex::detail::apply_parse_result(f, TLP{},
                                                                     value.template value_or_tag<
                                                                         TLP>(),
                                                                     lex::static_token<Tokens>(op)),
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
            template <class... Tokens>
            struct operator_spelling<lex::detail::type_list<Tokens...>>
            : operator_spelling<Tokens...>
            {};

            //=== operator parsers ===//
            template <class Parser, class TLP, class TokenSpec, class Func>
            constexpr auto parse(tokenizer<TokenSpec>& tokenizer, Func& f)
                -> parse_result<TLP, Func>
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

            template <class Production>
            struct atom : operand_parser
            {
                static_assert(is_production<Production>::value, "atom must be a production");

                using pre_tokens  = lex::detail::type_list<>;
                using post_tokens = lex::detail::type_list<>;

                template <class TLP, class TokenSpec, class Func>
                static constexpr auto parse_null(tokenizer<TokenSpec>& tokenizer, Func& f)
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
                static constexpr auto parse_left(tokenizer<TokenSpec>&, Func&,
                                                 const parse_result<TLP, Func>& lhs)
                {
                    return lhs;
                }
            };

            template <class TokenOpen, class TokenClose, class Atom>
            struct parenthesized : operand_parser
            {
                static_assert(is_token<TokenOpen>::value && is_token<TokenClose>::value,
                              "parentheses must be tokens");

                using pre_tokens  = lex::detail::type_list<>;
                using post_tokens = lex::detail::type_list<>;

                template <class TLP, class TokenSpec, class Func>
                static constexpr auto parse_null(tokenizer<TokenSpec>& tokenizer, Func& f)
                    -> parse_result<TLP, Func>
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

                        return value;
                    }
                    else
                        return Atom::template parse_null<TLP>(tokenizer, f);
                }

                template <class TLP, class TokenSpec, class Func>
                static constexpr auto parse_left(tokenizer<TokenSpec>&, Func&,
                                                 const parse_result<TLP, Func>& lhs)
                {
                    return lhs;
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
                using pre_tokens  = lex::detail::concat<typename Operator::token_list,
                                                       typename Operand::pre_tokens>;
                using post_tokens = typename Operand::post_tokens;

                template <class TLP, class TokenSpec, class Func>
                static constexpr auto parse_null(tokenizer<TokenSpec>& tokenizer, Func& f)
                    -> parse_result<TLP, Func>
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
                                                 parse_result<TLP, Func>& lhs)
                    -> parse_result<TLP, Func>
                {
                    return Operand::template parse_left<TLP>(tokenizer, f, lhs);
                }
            };

            template <associativity Assoc, class Operator, class Production, class Operand>
            struct prefix_prod : operand_parser
            {
                static_assert(is_production<Production>::value, "must be a production");

                using pre_tokens  = lex::detail::concat<typename Operator::token_list,
                                                       typename Operand::pre_tokens>;
                using post_tokens = typename Operand::post_tokens;

                template <class TLP, class TokenSpec, class Func>
                static constexpr auto parse_null(tokenizer<TokenSpec>& tokenizer, Func& f)
                    -> parse_result<TLP, Func>
                {
                    if (Operator::match(tokenizer.peek()))
                    {
                        auto op = Production::parse(tokenizer, f);
                        if (op.is_unmatched())
                            return {};

                        auto operand = Assoc == single ? parse<Operand, TLP>(tokenizer, f)
                                                       : parse<prefix_prod, TLP>(tokenizer, f);
                        if (operand.is_unmatched())
                            return operand;

                        return lex::detail::
                            apply_parse_result(f, TLP{}, op.template value_or_tag<Production>(),
                                               operand.template value_or_tag<TLP>());
                    }
                    else
                        return Operand::template parse_null<TLP>(tokenizer, f);
                }

                template <class TLP, class TokenSpec, class Func>
                static constexpr auto parse_left(tokenizer<TokenSpec>& tokenizer, Func& f,
                                                 parse_result<TLP, Func>& lhs)
                    -> parse_result<TLP, Func>
                {
                    return Operand::template parse_left<TLP>(tokenizer, f, lhs);
                }
            };

            template <associativity Assoc, class Operator, class Operand>
            struct postfix_op : operand_parser
            {
                using pre_tokens  = typename Operand::pre_tokens;
                using post_tokens = lex::detail::concat<typename Operator::token_list,
                                                        typename Operand::post_tokens>;

                template <class TLP, class TokenSpec, class Func>
                static constexpr auto parse_null(tokenizer<TokenSpec>& tokenizer, Func& f)
                    -> parse_result<TLP, Func>
                {
                    return Operand::template parse_null<TLP>(tokenizer, f);
                }

                template <class TLP, class TokenSpec, class Func>
                static constexpr auto parse_left(tokenizer<TokenSpec>& tokenizer, Func& f,
                                                 parse_result<TLP, Func>& lhs)
                    -> parse_result<TLP, Func>
                {
                    lhs = Operand::template parse_left<TLP>(tokenizer, f, lhs);
                    if (lhs.is_unmatched())
                        return lhs;

                    while (Operator::match(tokenizer.peek()))
                    {
                        auto op = tokenizer.get();
                        lhs     = Operator::apply_postfix(f, TLP{}, lhs, op);

                        if (Assoc == single)
                            break;
                    }

                    return lhs;
                }
            };

            template <associativity Assoc, class Operator, class Production, class Operand>
            struct postfix_prod : operand_parser
            {
                static_assert(is_production<Production>::value, "must be a production");

                using pre_tokens  = typename Operand::pre_tokens;
                using post_tokens = lex::detail::concat<typename Operator::token_list,
                                                        typename Operand::post_tokens>;

                template <class TLP, class TokenSpec, class Func>
                static constexpr auto parse_null(tokenizer<TokenSpec>& tokenizer, Func& f)
                    -> parse_result<TLP, Func>
                {
                    return Operand::template parse_null<TLP>(tokenizer, f);
                }

                template <class TLP, class TokenSpec, class Func>
                static constexpr auto parse_left(tokenizer<TokenSpec>& tokenizer, Func& f,
                                                 parse_result<TLP, Func>& lhs)
                    -> parse_result<TLP, Func>
                {
                    lhs = Operand::template parse_left<TLP>(tokenizer, f, lhs);
                    if (lhs.is_unmatched())
                        return lhs;

                    while (Operator::match(tokenizer.peek()))
                    {
                        auto op = Production::parse(tokenizer, f);
                        if (op.is_unmatched())
                            return {};

                        lhs = lex::detail::apply_parse_result(f, TLP{},
                                                              lhs.template value_or_tag<TLP>(),
                                                              op.template value_or_tag<
                                                                  Production>());

                        if (Assoc == single)
                            break;
                    }

                    return lhs;
                }
            };

            template <associativity Assoc, class Operator, class Operand>
            struct binary_op : operand_parser
            {
                using pre_tokens  = typename Operand::pre_tokens;
                using post_tokens = lex::detail::concat<typename Operator::token_list,
                                                        typename Operand::post_tokens>;

                template <class TLP, class TokenSpec, class Func>
                static constexpr auto parse_null(tokenizer<TokenSpec>& tokenizer, Func& f)
                    -> parse_result<TLP, Func>
                {
                    return Operand::template parse_null<TLP>(tokenizer, f);
                }

                template <class TLP, class TokenSpec, class Func>
                static constexpr auto parse_left(tokenizer<TokenSpec>& tokenizer, Func& f,
                                                 parse_result<TLP, Func>& lhs)
                    -> parse_result<TLP, Func>
                {
                    lhs = Operand::template parse_left<TLP>(tokenizer, f, lhs);
                    if (lhs.is_unmatched())
                        return lhs;

                    while (Operator::match(tokenizer.peek()))
                    {
                        auto op = tokenizer.get();

                        auto operand = Assoc == right ? parse<binary_op, TLP>(tokenizer, f)
                                                      : parse<Operand, TLP>(tokenizer, f);
                        if (operand.is_unmatched())
                            return operand;

                        lhs = Operator::apply_binary(f, TLP{}, lhs, op, operand);
                        if (Assoc == single && Operator::match(op))
                            break;
                    }

                    return lhs;
                }
            };

            template <associativity Assoc, class Operator, class Production, class Operand>
            struct binary_prod : operand_parser
            {
                static_assert(is_production<Production>::value, "must be a production");

                using pre_tokens  = typename Operand::pre_tokens;
                using post_tokens = lex::detail::concat<typename Operator::token_list,
                                                        typename Operand::post_tokens>;

                template <class TLP, class TokenSpec, class Func>
                static constexpr auto parse_null(tokenizer<TokenSpec>& tokenizer, Func& f)
                    -> parse_result<TLP, Func>
                {
                    return Operand::template parse_null<TLP>(tokenizer, f);
                }

                template <class TLP, class TokenSpec, class Func>
                static constexpr auto parse_left(tokenizer<TokenSpec>& tokenizer, Func& f,
                                                 parse_result<TLP, Func>& lhs)
                    -> parse_result<TLP, Func>
                {
                    lhs = Operand::template parse_left<TLP>(tokenizer, f, lhs);
                    if (lhs.is_unmatched())
                        return lhs;

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

                        lhs = lex::detail::apply_parse_result(f, TLP{},
                                                              lhs.template value_or_tag<TLP>(),
                                                              op.template value_or_tag<
                                                                  Production>(),
                                                              operand.template value_or_tag<TLP>());
                        if (Assoc == single && Operator::match(op_token))
                            break;
                    }

                    return lhs;
                }
            };

            template <class Child, class... Children>
            struct rule : operator_adl
            {
                using pre_tokens  = lex::detail::concat<typename Child::pre_tokens,
                                                       typename Children::pre_tokens...>;
                using post_tokens = lex::detail::concat<typename Child::post_tokens,
                                                        typename Children::post_tokens...>;

                static_assert(
                    lex::detail::is_unique<pre_tokens>::value,
                    "operator choice cannot uniquely decide a path based on a prefix operator, "
                    "try extracting common operands as a separate production");

                template <class TLP, class TokenSpec, class Func>
                static constexpr auto parse_left_impl(lex::detail::type_list<>,
                                                      tokenizer<TokenSpec>&, Func&,
                                                      parse_result<TLP, Func>& lhs)
                {
                    // no left operator found
                    return lhs;
                }
                template <class TLP, class Head, class... Tail, class TokenSpec, class Func>
                static constexpr auto parse_left_impl(lex::detail::type_list<Head, Tail...>,
                                                      tokenizer<TokenSpec>& tokenizer, Func& f,
                                                      parse_result<TLP, Func>& lhs)
                {
                    auto old    = tokenizer.current_ptr();
                    auto result = Head::template parse_left<TLP>(tokenizer, f, lhs);
                    if (tokenizer.current_ptr() != old)
                        // Head parsed something, so we're done
                        return result;
                    else
                        // try the next child
                        return parse_left_impl<TLP>(lex::detail::type_list<Tail...>{}, tokenizer, f,
                                                    lhs);
                }

                template <class TLP, class TokenSpec, class Func>
                static constexpr auto parse_impl(lex::detail::type_list<>,
                                                 tokenizer<TokenSpec>& tokenizer, Func& f)
                {
                    // we haven't found any prefix operator, so parse an atom
                    // then use the first child whose parse_left() function matched something

                    // to parse an atom, we can just use the parse_null() function of any child,
                    // as we didn't have a prefix operator that would match
                    auto atom = Child::template parse_null<TLP>(tokenizer, f);
                    return parse_left_impl<TLP>(lex::detail::type_list<Child, Children...>{},
                                                tokenizer, f, atom);
                }
                template <class TLP, class Head, class... Tail, class TokenSpec, class Func>
                static constexpr auto parse_impl(lex::detail::type_list<Head, Tail...>,
                                                 tokenizer<TokenSpec>& tokenizer, Func& f)
                {
                    if (operator_spelling<typename Head::pre_tokens>::match(tokenizer.peek()))
                        // we have a prefix operator that definitely narrows it down to Head
                        return detail::parse<Head, TLP>(tokenizer, f);
                    else
                        // try to parse the next one instead
                        return parse_impl<TLP>(lex::detail::type_list<Tail...>{}, tokenizer, f);
                }

                template <class TLP, class TokenSpec, class Func>
                static constexpr auto parse(tokenizer<TokenSpec>& tokenizer, Func& f)
                {
                    return parse_impl<TLP>(lex::detail::type_list<Child, Children...>{}, tokenizer,
                                           f);
                }
            };

            template <class Rule>
            struct end : operator_adl
            {
                template <class TLP, class TokenSpec, class Func>
                static constexpr auto parse(tokenizer<TokenSpec>& tokenizer, Func& f)
                    -> parse_result<TLP, Func>
                {
                    auto result = Rule::template parse<TLP>(tokenizer, f);
                    if (result.is_unmatched())
                        return result;
                    else if (operator_spelling<typename Rule::post_tokens>::match(tokenizer.peek()))
                    {
                        auto error = illegal_operator_chain<typename TLP::grammar, TLP>(TLP{});
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
                static constexpr auto verify
                    = (&detail::verify_operand<Operand1>, &detail::verify_operand<Operand2>);
                using type = rule<Operand1, Operand2>;
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

        template <class Production>
        constexpr auto atom = detail::atom<Production>{};

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
            -> operator_rule::detail::parse_result<Derived, Func>
        {
            using rule = operator_rule::detail::make_rule<decltype(Derived::rule())>;
            return rule::template parse<Derived>(tokenizer, f);
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
