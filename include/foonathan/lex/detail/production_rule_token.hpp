// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_PRODUCTION_RULE_TOKEN_HPP_INCLUDED
#define FOONATHAN_LEX_PRODUCTION_RULE_TOKEN_HPP_INCLUDED

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
            struct base_token_rule : production_rule::base_rule
            {};
            template <typename T>
            struct is_token_rule : std::is_base_of<base_token_rule, T>
            {};

            template <class Token>
            struct silent_token;
            template <class... Tokens>
            struct token_sequence;
            template <class... Tokens>
            struct token_choice;

            template <class Token>
            struct token : base_token_rule
            {
                static_assert(is_token<Token>::value, "only a token can be used in this context");

                using silence = silent_token<Token>;

                template <class Other>
                using sequence_with = token_sequence<token<Token>, Other>;
                template <class Other>
                using choice_with = token_choice<token<Token>, Other>;

                template <class TokenSpec>
                static constexpr bool peek(const tokenizer<TokenSpec>& tokenizer)
                {
                    return tokenizer.peek().is(Token{});
                }

                template <class Cont>
                struct parser : Cont
                {
                    using grammar = typename Cont::grammar;
                    using tlp     = typename Cont::tlp;

                    template <class TokenSpec, typename Func, typename... Args>
                    static constexpr auto parse(tokenizer<TokenSpec>& tokenizer, Func& f,
                                                Args&&... args)
                        -> decltype(Cont::parse(tokenizer, f, static_cast<Args&&>(args)...,
                                                static_token<Token>(tokenizer.get())))
                    {
                        auto token = tokenizer.peek();
                        if (token.is(Token{}))
                        {
                            tokenizer.bump();
                            return Cont::parse(tokenizer, f, static_cast<Args&&>(args)...,
                                               static_token<Token>(token));
                        }
                        else
                        {
                            auto error
                                = unexpected_token_error<grammar, tlp, Token>(tlp{}, Token{});
                            lex::detail::report_error(f, error, tokenizer);
                            return {};
                        }
                    }
                };
            };

            template <class Token>
            struct silent_token : base_token_rule
            {
                static_assert(is_token<Token>::value, "only a token can be used in this context");

                using silence = silent_token<Token>;

                template <class Other>
                using sequence_with = token_sequence<silent_token<Token>, Other>;
                template <class Other>
                using choice_with = token_choice<silent_token<Token>, Other>;

                template <class TokenSpec>
                static constexpr bool peek(const tokenizer<TokenSpec>& tokenizer)
                {
                    return tokenizer.peek().is(Token{});
                }

                template <class Cont>
                struct parser : Cont
                {
                    using grammar = typename Cont::grammar;
                    using tlp     = typename Cont::tlp;

                    template <class TokenSpec, typename Func, typename... Args>
                    static constexpr auto parse(tokenizer<TokenSpec>& tokenizer, Func& f,
                                                Args&&... args)
                        -> decltype(Cont::parse(tokenizer, f, static_cast<Args&&>(args)...))
                    {
                        auto token = tokenizer.peek();
                        if (token.is(Token{}))
                        {
                            tokenizer.bump();
                            return Cont::parse(tokenizer, f, static_cast<Args&&>(args)...);
                        }
                        else
                        {
                            auto error
                                = unexpected_token_error<grammar, tlp, Token>(tlp{}, Token{});
                            lex::detail::report_error(f, error, tokenizer);
                            return {};
                        }
                    }
                };
            };

            template <class... Tokens>
            struct token_sequence : base_token_rule
            {
                static_assert(
                    lex::detail::all_of<lex::detail::type_list<Tokens...>, is_token_rule>::value,
                    "only a token can be used in this context");

                using silence = token_sequence<typename Tokens::silence...>;

                template <class Other>
                using sequence_with = token_sequence<Tokens..., Other>;
                template <class Other>
                using choice_with = token_choice<token_sequence<Tokens...>, Other>;

                template <class TokenSpec>
                static constexpr bool peek_impl(token_sequence<>, const tokenizer<TokenSpec>&)
                {
                    return true;
                }
                template <class Head, class... Tail, class TokenSpec>
                static constexpr bool peek_impl(token_sequence<Head, Tail...>,
                                                const tokenizer<TokenSpec>& tokenizer)
                {
                    return Head::peek(tokenizer);
                }

                template <class TokenSpec>
                static constexpr bool peek(const tokenizer<TokenSpec>& tokenizer)
                {
                    return peek_impl(token_sequence<Tokens...>{}, tokenizer);
                }

                template <class Cont>
                using parser = parser_for<Tokens..., Cont>;
            };

            template <class... Tokens>
            struct token_choice : base_token_rule
            {
                static_assert(
                    lex::detail::all_of<lex::detail::type_list<Tokens...>, is_token_rule>::value,
                    "only a token can be used in this context");

                using silence = token_choice<typename Tokens::silence...>;

                template <class Other>
                using sequence_with = token_sequence<token_choice<Tokens...>, Other>;
                template <class Other>
                using choice_with = token_choice<Tokens..., Other>;

                template <class TokenSpec>
                static constexpr bool peek_impl(token_choice<>, const tokenizer<TokenSpec>&)
                {
                    return false;
                }
                template <class Head, class... Tail, class TokenSpec>
                static constexpr bool peek_impl(token_choice<Head, Tail...>,
                                                const tokenizer<TokenSpec>& tokenizer)
                {
                    return Head::peek(tokenizer) || peek_impl(token_choice<Tail...>{}, tokenizer);
                }

                template <class TokenSpec>
                static constexpr bool peek(const tokenizer<TokenSpec>& tokenizer)
                {
                    return peek_impl(token_choice<Tokens...>{}, tokenizer);
                }

                template <class Cont>
                struct parser : Cont
                {
                    // TODO: add  token_choice<> overload and report no alternative
                    template <class Head, class TokenSpec, typename Func, typename... Args>
                    static constexpr auto parse_impl(int, token_choice<Head>,
                                                     tokenizer<TokenSpec>& tokenizer, Func& f,
                                                     Args&&... args)
                    {
                        return parser_for<Head, Cont>::parse(tokenizer, f,
                                                             static_cast<Args&&>(args)...);
                    }
                    template <class Head, class... Tail, class TokenSpec, typename Func,
                              typename... Args>
                    static constexpr auto parse_impl(short, token_choice<Head, Tail...>,
                                                     tokenizer<TokenSpec>& tokenizer, Func& f,
                                                     Args&&... args)
                    {
                        if (Head::peek(tokenizer))
                            return parser_for<Head, Cont>::parse(tokenizer, f,
                                                                 static_cast<Args&&>(args)...);
                        else
                            return parse_impl(0, token_choice<Tail...>{}, tokenizer, f,
                                              static_cast<Args&&>(args)...);
                    }

                    template <class TokenSpec, typename Func, typename... Args>
                    static constexpr auto parse(tokenizer<TokenSpec>& tokenizer, Func& f,
                                                Args&&... args)
                    {
                        return parse_impl(0, token_choice<Tokens...>{}, tokenizer, f,
                                          static_cast<Args&&>(args)...);
                    }
                };
            };
        } // namespace detail
    }     // namespace production_rule
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_PRODUCTION_RULE_TOKEN_HPP_INCLUDED
