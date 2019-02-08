// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_LITERAL_TOKEN_HPP_INCLUDED
#define FOONATHAN_LEX_LITERAL_TOKEN_HPP_INCLUDED

#include <foonathan/lex/detail/string.hpp>
#include <foonathan/lex/token_spec.hpp>

namespace foonathan
{
namespace lex
{
    /// A literal token.
    ///
    /// It matches the given sequence of characters.
    /// If there are multiple literal tokens with a common prefix the longest matching will be
    /// selected.
    template <char... Literal>
    struct literal_token : detail::base_token
    {
        static constexpr const char value[sizeof...(Literal) + 1] = {Literal..., '\0'};
        static_assert(value[sizeof...(Literal) - 1] != '\0', "literal must not be null-terminated");

        static constexpr const char* name = value;
    };

    template <char... Literal>
    constexpr const char literal_token<Literal...>::value[];

    /// Expands to `literal_token<String[0], String[1], ...>`.
    /// It ignores all null characters.
#define FOONATHAN_LEX_LITERAL(String)                                                              \
    FOONATHAN_LEX_DETAIL_STRING(foonathan::lex::literal_token, String)

    namespace detail
    {
        template <template <char...> class Kind, class Token>
        struct is_literal_token_impl
        {
            template <char... Literal>
            static std::true_type  test(const Kind<Literal...>&);
            static std::false_type test(...);

            using value = decltype(test(std::declval<Token>()));
        };

        template <char... Literal>
        literal_token<Literal...> get_literal_type(const literal_token<Literal...>&);

        template <class Token>
        using literal_token_type = decltype(get_literal_type(Token{}));
    } // namespace detail

    /// Whether or not the token is a literal token.
    template <class Token>
    struct is_literal_token : detail::is_literal_token_impl<literal_token, Token>::value
    {};
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_LITERAL_TOKEN_HPP_INCLUDED
