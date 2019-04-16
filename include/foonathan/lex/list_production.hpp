// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_LIST_PRODUCTION_HPP_INCLUDED
#define FOONATHAN_LEX_LIST_PRODUCTION_HPP_INCLUDED

#include <foonathan/lex/grammar.hpp>
#include <foonathan/lex/parse_error.hpp>
#include <foonathan/lex/parse_result.hpp>
#include <foonathan/lex/tokenizer.hpp>

namespace foonathan
{
namespace lex
{
    template <class Derived, class Grammar, class Production, class Separator>
    class list_production : public detail::base_production
    {
        static_assert(is_production<Production>::value, "list element must be a production");
        static_assert(is_token<Separator>::value, "list separator must be a token");

        template <class End, class Func>
        static constexpr auto parse_impl(std::true_type /* allow_empty */,
                                         tokenizer<typename Grammar::token_spec>& tokenizer,
                                         Func&&                                   f)
        {
            auto result = lex::detail::apply_parse_result(f, Derived{});
            if (tokenizer.peek().is(End{}))
                return result;

            auto elem = Production::parse(tokenizer, f);
            if (elem.is_unmatched())
                return decltype(result)::unmatched();

            result
                = lex::detail::apply_parse_result(f, Derived{}, result.template forward<Derived>(),
                                                  elem.template forward<Production>());

            while (tokenizer.peek().is(Separator{}))
            {
                tokenizer.bump();
                if (Derived::allow_trailing::value && tokenizer.peek().is(End{}))
                    break;

                elem = Production::parse(tokenizer, f);
                if (elem.is_unmatched())
                    return decltype(result)::unmatched();

                result = lex::detail::apply_parse_result(f, Derived{},
                                                         result.template forward<Derived>(),
                                                         elem.template forward<Production>());
            }

            return result;
        }

        template <class End, class Func>
        static constexpr auto parse_impl(std::false_type /* allow_empty */,
                                         tokenizer<typename Grammar::token_spec>& tokenizer,
                                         Func&&                                   f)
            -> decltype(lex::detail::apply_parse_result(
                f, std::declval<Derived>(),
                Production::parse(tokenizer, f).template forward<Production>()))
        {
            auto elem = Production::parse(tokenizer, f);
            if (elem.is_unmatched())
                return {};

            auto result = lex::detail::apply_parse_result(f, Derived{},
                                                          elem.template forward<Production>());

            while (tokenizer.peek().is(Separator{}))
            {
                tokenizer.bump();
                if (Derived::allow_trailing::value && tokenizer.peek().is(End{}))
                    break;

                elem = Production::parse(tokenizer, f);
                if (elem.is_unmatched())
                    return {};

                result = lex::detail::apply_parse_result(f, Derived{},
                                                         result.template forward<Derived>(),
                                                         elem.template forward<Production>());
            }

            return result;
        }

    public:
        using end_token      = void;
        using allow_empty    = std::false_type;
        using allow_trailing = std::false_type;

        template <class Func>
        static constexpr auto parse(tokenizer<typename Grammar::token_spec>& tokenizer, Func&& f)
        {
            constexpr bool has_end = !std::is_same<typename Derived::end_token, void>::value;
            static_assert(!Derived::allow_empty::value || has_end,
                          "an empty list requires an end token");
            static_assert(!Derived::allow_trailing::value || has_end,
                          "a list with trailing seperator requires an end token");

            using end = std::conditional_t<has_end, typename Derived::end_token, lex::eof_token>;
            return parse_impl<end>(typename Derived::allow_empty{}, tokenizer, f);
        }
    };

    template <class Derived, class Grammar, class Open, class Production, class Separator,
              class Close>
    class bracketed_list_production : public detail::base_production
    {
        static_assert(is_token<Open>::value && is_token<Close>::value,
                      "list brackets must be tokens");

    public:
        using end_token      = Close;
        using allow_empty    = std::false_type;
        using allow_trailing = std::false_type;

        template <class Func>
        static constexpr auto parse(tokenizer<typename Grammar::token_spec>& tokenizer, Func&& f)
            -> decltype(list_production<Derived, Grammar, Production, Separator>::parse(tokenizer,
                                                                                        f))
        {
            static_assert(std::is_same<typename Derived::end_token, Close>::value,
                          "must not override end_token");

            if (tokenizer.peek().is(Open{}))
                tokenizer.bump();
            else
            {
                auto error = lex::unexpected_token<Grammar, Derived, Open>(Derived{}, Open{});
                lex::detail::report_error(f, error, tokenizer);
                return {};
            }

            auto result
                = list_production<Derived, Grammar, Production, Separator>::parse(tokenizer, f);
            if (result.is_unmatched())
                return result;

            if (tokenizer.peek().is(Close{}))
                tokenizer.bump();
            else
            {
                auto error = lex::unexpected_token<Grammar, Derived, Close>(Derived{}, Close{});
                lex::detail::report_error(f, error, tokenizer);
                return {};
            }

            return result;
        }
    };
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_LIST_PRODUCTION_HPP_INCLUDED
