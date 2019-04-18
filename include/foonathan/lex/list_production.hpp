// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_LIST_PRODUCTION_HPP_INCLUDED
#define FOONATHAN_LEX_LIST_PRODUCTION_HPP_INCLUDED

#include <foonathan/lex/grammar.hpp>
#include <foonathan/lex/parser.hpp>
#include <foonathan/lex/tokenizer.hpp>

namespace foonathan
{
namespace lex
{
    namespace detail
    {
        template <class TLP, class Grammar>
        struct list_production_impl
        {
            //=== parse_elem ===//
            template <class Production, class Func,
                      std::enable_if_t<is_production<Production>::value, int> = 0>
            static constexpr auto parse_elem(tokenizer<typename Grammar::token_spec>& tokenizer,
                                             Func&&                                   f)
            {
                return Production::parse(tokenizer, f);
            }
            template <class Token, class Func, std::enable_if_t<is_token<Token>::value, short> = 0>
            static constexpr auto parse_elem(tokenizer<typename Grammar::token_spec>& tokenizer,
                                             Func&&                                   f)
            {
                auto token        = tokenizer.peek();
                using result_type = parse_result<decltype(detail::parse_token<Token>(token))>;
                if (token.is(Token{}))
                {
                    tokenizer.bump();
                    return result_type::success(detail::parse_token<Token>(token));
                }
                else
                {
                    auto error = unexpected_token<Grammar, TLP, Token>(TLP{}, Token{});
                    detail::report_error(f, error, tokenizer);
                    return result_type::unmatched();
                }
            }
            template <class Element>
            static constexpr auto parse_elem(...)
            {
                static_assert(sizeof(Element) != sizeof(Element),
                              "list element must be a token or a production");
                return parse_result<void>();
            }

            //=== parse_separator ===//
            template <class Separator, class End,
                      std::enable_if_t<is_token<Separator>::value, int> = 0>
            static constexpr bool parse_separator(
                bool allow_trailing, tokenizer<typename Grammar::token_spec>& tokenizer)
            {
                static_assert(is_token<End>::value, "list end must be a token");
                if (tokenizer.peek().is(Separator{}))
                {
                    tokenizer.bump();
                    if (allow_trailing && tokenizer.peek().is(End{}))
                        return false;
                    else
                        return true;
                }
                else
                    return false;
            }
            template <class Separator, class End,
                      std::enable_if_t<std::is_same<Separator, void>::value, short> = 0>
            static constexpr bool parse_separator(
                bool, tokenizer<typename Grammar::token_spec>& tokenizer)
            {
                static_assert(is_token<End>::value, "list end must be a token");
                return !tokenizer.peek().is(End{});
            }
            template <class Separator, class End>
            static constexpr bool parse_separator(...)
            {
                static_assert(sizeof(Separator) != sizeof(Separator),
                              "list separator must be a token");
                static_assert(is_token<End>::value, "list end must be a token");
                return true;
            }

            //=== parser ==//
            template <class Element, class Separator, class End, bool AllowTrailing>
            struct empty_parser
            {
                template <class Func>
                static constexpr auto parse(tokenizer<typename Grammar::token_spec>& tokenizer,
                                            Func&&                                   f)
                {
                    auto result = lex::detail::apply_parse_result(f, TLP{});
                    if (tokenizer.peek().is(End{}))
                        return result;

                    auto elem = parse_elem<Element>(tokenizer, f);
                    if (elem.is_unmatched())
                        return decltype(result)::unmatched();

                    result
                        = lex::detail::apply_parse_result(f, TLP{}, result.template forward<TLP>(),
                                                          elem.template forward<Element>());

                    while (parse_separator<Separator, End>(AllowTrailing, tokenizer))
                    {
                        elem = parse_elem<Element>(tokenizer, f);
                        if (elem.is_unmatched())
                            return decltype(result)::unmatched();

                        result = lex::detail::apply_parse_result(f, TLP{},
                                                                 result.template forward<TLP>(),
                                                                 elem.template forward<Element>());
                    }

                    return result;
                }
            };

            template <class Element, class Seperator, class End, bool AllowTrailing>
            struct non_empty_parser
            {
                template <class Func>
                static constexpr auto parse(tokenizer<typename Grammar::token_spec>& tokenizer,
                                            Func&&                                   f)
                {
                    auto elem         = parse_elem<Element>(tokenizer, f);
                    using return_type = decltype(
                        lex::detail::apply_parse_result(f, TLP{},
                                                        elem.template forward<Element>()));
                    if (elem.is_unmatched())
                        return return_type{};

                    auto result = lex::detail::apply_parse_result(f, TLP{},
                                                                  elem.template forward<Element>());

                    while (parse_separator<Seperator, End>(AllowTrailing, tokenizer))
                    {
                        elem = parse_elem<Element>(tokenizer, f);
                        if (elem.is_unmatched())
                            return return_type{};

                        result = lex::detail::apply_parse_result(f, TLP{},
                                                                 result.template forward<TLP>(),
                                                                 elem.template forward<Element>());
                    }

                    return result;
                }
            };
        };
    } // namespace detail

    template <class Derived, class Grammar>
    class list_production : public detail::base_production
    {
    public:
        using element = void;

        using separator_token = void;
        using end_token       = void;
        using allow_empty     = std::false_type;
        using allow_trailing  = std::false_type;

        template <class Func>
        static constexpr auto parse(tokenizer<typename Grammar::token_spec>& tokenizer, Func&& f)
        {
            constexpr bool has_end = !std::is_same<typename Derived::end_token, void>::value;
            constexpr bool without_seperator
                = std::is_same<typename Derived::separator_token, void>::value;
            static_assert(!without_seperator || has_end,
                          "a list without separator requires ::end_token");
            static_assert(!Derived::allow_empty::value || has_end,
                          "an empty list requires ::end_token");
            static_assert(!Derived::allow_trailing::value || has_end,
                          "a list with trailing seperator requires ::end_token");

            using impl      = detail::list_production_impl<Derived, Grammar>;
            using element   = typename Derived::element;
            using separator = typename Derived::separator_token;
            using end = std::conditional_t<has_end, typename Derived::end_token, lex::eof_token>;
            using parser
                = std::conditional_t<Derived::allow_empty::value,
                                     typename impl::template empty_parser<
                                         element, separator, end, Derived::allow_trailing::value>,
                                     typename impl::template non_empty_parser<
                                         element, separator, end, Derived::allow_trailing::value>>;
            return parser::parse(tokenizer, f);
        }
    };

    template <class Derived, class Grammar>
    class bracketed_list_production : public detail::base_production
    {
    public:
        using element       = void;
        using open_bracket  = void;
        using close_bracket = void;

        using separator_token = void;
        using allow_empty     = std::false_type;
        using allow_trailing  = std::false_type;

        template <class Func>
        static constexpr auto parse(tokenizer<typename Grammar::token_spec>& tokenizer, Func&& f)
        {
            using open  = typename Derived::open_bracket;
            using close = typename Derived::close_bracket;
            static_assert(is_token<open>::value && is_token<close>::value,
                          "list brackets must be tokens");

            using impl      = detail::list_production_impl<Derived, Grammar>;
            using element   = typename Derived::element;
            using separator = typename Derived::separator_token;
            using parser    = std::conditional_t<
                Derived::allow_empty::value,
                typename impl::template empty_parser<element, separator, close,
                                                     Derived::allow_trailing::value>,
                typename impl::template non_empty_parser<element, separator, close,
                                                         Derived::allow_trailing::value>>;

            if (tokenizer.peek().is(open{}))
                tokenizer.bump();
            else
            {
                auto error = lex::unexpected_token<Grammar, Derived, open>(Derived{}, open{});
                lex::detail::report_error(f, error, tokenizer);
                return decltype(parser::parse(tokenizer, f))::unmatched();
            }

            auto result = parser::parse(tokenizer, f);
            if (result.is_unmatched())
                return result;

            if (tokenizer.peek().is(close{}))
                tokenizer.bump();
            else
            {
                auto error = lex::unexpected_token<Grammar, Derived, close>(Derived{}, close{});
                lex::detail::report_error(f, error, tokenizer);
                return decltype(parser::parse(tokenizer, f))::unmatched();
            }

            return result;
        }
    };
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_LIST_PRODUCTION_HPP_INCLUDED
