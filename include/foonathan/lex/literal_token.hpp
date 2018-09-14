// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_LITERAL_TOKEN_HPP_INCLUDED
#define FOONATHAN_LEX_LITERAL_TOKEN_HPP_INCLUDED

#include <foonathan/lex/token.hpp>

namespace foonathan
{
    namespace lex
    {
        template <char... Literal>
        struct literal_token
        {
            static constexpr const char value[] = {Literal..., '\0'};
            static_assert(value[sizeof...(Literal) - 1] != '\0',
                          "literal must not be null-terminated");
        };

        namespace detail
        {
            template <class Token>
            struct is_literal_token_impl
            {
                template <char... Literal>
                static std::true_type  test(const literal_token<Literal...>&);
                static std::false_type test(...);

                using value = decltype(test(std::declval<Token>()));
            };
        } // namespace detail

        /// Whether or not the token is a literal token.
        template <class Token>
        struct is_literal_token : detail::is_literal_token_impl<Token>::value
        {
        };
    } // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_LITERAL_TOKEN_HPP_INCLUDED
