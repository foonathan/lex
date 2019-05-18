// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_IDENTIFIER_HPP_INCLUDED
#define FOONATHAN_LEX_IDENTIFIER_HPP_INCLUDED

#include <foonathan/lex/literal_token.hpp>
#include <foonathan/lex/rule_token.hpp>

namespace foonathan
{
namespace lex
{
    template <class Derived, class TokenSpec>
    struct identifier_token : rule_token<Derived, TokenSpec>
    {
        static constexpr const char* name = "<identifier>";
    };

    template <class Token>
    struct is_identifier_token : detail::is_token_impl<identifier_token, Token>::value
    {};

    template <class Token>
    struct is_non_identifier_rule_token
    : std::integral_constant<bool,
                             is_rule_token<Token>::value && !is_identifier_token<Token>::value>
    {};

    template <char... Char>
    struct keyword_token : literal_token<Char...>
    {};

#define FOONATHAN_LEX_KEYWORD(String)                                                              \
    FOONATHAN_LEX_DETAIL_STRING(foonathan::lex::keyword_token, String)

    template <class Token>
    struct is_keyword_token : detail::is_literal_token_impl<keyword_token, Token>::value
    {};

    template <class Token>
    struct is_non_keyword_literal_token
    : std::integral_constant<bool,
                             is_literal_token<Token>::value && !is_keyword_token<Token>::value>
    {};
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_IDENTIFIER_HPP_INCLUDED
