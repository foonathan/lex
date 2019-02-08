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
    /// An identifier.
    ///
    /// It is a [lex::rule_token]() but with special support for [lex::keyword]().
    /// At most one identifier must be present in a token specification.
    template <class Derived, class TokenSpec>
    struct identifier_token : rule_token<Derived, TokenSpec>
    {
        static constexpr const char* name = "<identifier>";
    };

    /// Whether or not the given token is an identifier.
    template <class Token>
    struct is_identifier_token : detail::is_token_impl<identifier_token, Token>::value
    {};

    /// Whether or not the given token is a rule token that is not an identifier.
    template <class Token>
    struct is_non_identifier_rule_token
    : std::integral_constant<bool,
                             is_rule_token<Token>::value && !is_identifier_token<Token>::value>
    {};

    /// A keyword.
    ///
    /// This is a special [lex::literal_token]().
    /// If it is present in a token specification, the literal token must be present as well.
    /// It matches any literal that consists of this exact sequence of characters.
    template <char... Char>
    struct keyword_token : literal_token<Char...>
    {};

    /// Expands to `keyword<String[0], String[1], ...>`.
    /// It ignores all null characters.
#define FOONATHAN_LEX_KEYWORD(String)                                                              \
    FOONATHAN_LEX_DETAIL_STRING(foonathan::lex::keyword_token, String)

    /// Whether or not the given token is a keyword.
    template <class Token>
    struct is_keyword_token : detail::is_literal_token_impl<keyword_token, Token>::value
    {};

    /// Whether or not the given token is a literal token that is not a keyword.
    template <class Token>
    struct is_non_keyword_literal_token
    : std::integral_constant<bool,
                             is_literal_token<Token>::value && !is_keyword_token<Token>::value>
    {};
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_IDENTIFIER_HPP_INCLUDED
