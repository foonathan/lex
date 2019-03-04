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

                using peek_tokens = lex::detail::type_list<Token>;

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
                            auto error = unexpected_token<grammar, tlp, Token>(tlp{}, Token{});
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

                using peek_tokens = lex::detail::type_list<Token>;

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
                            auto error = unexpected_token<grammar, tlp, Token>(tlp{}, Token{});
                            lex::detail::report_error(f, error, tokenizer);
                            return {};
                        }
                    }
                };
            };

            template <class... Tokens>
            struct token_sequence;
            template <>
            struct token_sequence<> : base_token_rule
            {
                using silence = token_sequence<>;

                template <class Other>
                using sequence_with = token_sequence<Other>;
                template <class Other>
                using choice_with = token_choice<token_sequence<>, Other>;

                using peek_tokens = lex::detail::type_list<>;

                template <class TokenSpec>
                static constexpr bool peek(const tokenizer<TokenSpec>&)
                {
                    return true;
                }

                template <class Cont>
                using parser = Cont;
            };
            template <class Head, class... Tail>
            struct token_sequence<Head, Tail...> : base_token_rule
            {
                static_assert(lex::detail::all_of<lex::detail::type_list<Head, Tail...>,
                                                  is_token_rule>::value,
                              "only a token can be used in this context");

                using silence = token_sequence<typename Head::silence, typename Tail::silence...>;

                template <class Other>
                using sequence_with = token_sequence<Head, Tail..., Other>;
                template <class Other>
                using choice_with = token_choice<token_sequence<Head, Tail...>, Other>;

                using peek_tokens = typename Head::peek_tokens;

                template <class TokenSpec>
                static constexpr bool peek(const tokenizer<TokenSpec>& tokenizer)
                {
                    return Head::peek(tokenizer);
                }

                template <class Cont>
                using parser = parser_for<Head, Tail..., Cont>;
            };

            template <class... Choices>
            struct token_choice : base_token_rule
            {
                static_assert(
                    lex::detail::all_of<lex::detail::type_list<Choices...>, is_token_rule>::value,
                    "only a token can be used in this context");

                using silence = token_choice<typename Choices::silence...>;

                template <class Other>
                using sequence_with = token_sequence<token_choice<Choices...>, Other>;
                template <class Other>
                using choice_with = token_choice<Choices..., Other>;

                using peek_tokens = lex::detail::concat<lex::detail::type_list<>,
                                                        typename Choices::peek_tokens...>;

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
                    return peek_impl(token_choice<Choices...>{}, tokenizer);
                }

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
                    template <class Token, class TokenSpec, typename Func>
                    static constexpr void report_error(lex::detail::type_list<Token>,
                                                       tokenizer<TokenSpec>& tokenizer, Func& f)
                    {
                        // if there is only a single token this is an unexpected token error
                        auto error = unexpected_token<grammar, tlp, Token>(tlp{}, Token{});
                        lex::detail::report_error(f, error, tokenizer);
                    }

                    template <class R, class TokenSpec, typename Func, typename... Args>
                    static constexpr R parse_impl(token_choice<>, tokenizer<TokenSpec>& tokenizer,
                                                  Func& f, Args&&...)
                    {
                        report_error(peek_tokens{}, tokenizer, f);
                        return {};
                    }
                    template <class R, class Head, class... Tail, class TokenSpec, typename Func,
                              typename... Args>
                    static constexpr R parse_impl(token_choice<Head, Tail...>,
                                                  tokenizer<TokenSpec>& tokenizer, Func& f,
                                                  Args&&... args)
                    {
                        if (Head::peek(tokenizer))
                            return parser_for<Head, Cont>::parse(tokenizer, f,
                                                                 static_cast<Args&&>(args)...);
                        else
                            return parse_impl<R>(token_choice<Tail...>{}, tokenizer, f,
                                                 static_cast<Args&&>(args)...);
                    }

                    template <class TokenSpec, typename Func, typename... Args>
                    static constexpr auto parse(tokenizer<TokenSpec>& tokenizer, Func& f,
                                                Args&&... args)
                    {
                        using return_type = std::common_type_t<decltype(
                            parser_for<Choices, Cont>::parse(tokenizer, f,
                                                             static_cast<Args&&>(args)...))...>;
                        return parse_impl<return_type>(token_choice<Choices...>{}, tokenizer, f,
                                                       static_cast<Args&&>(args)...);
                    }
                };
            };
        } // namespace detail
    }     // namespace production_rule
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_PRODUCTION_RULE_TOKEN_HPP_INCLUDED
