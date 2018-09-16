// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
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
    struct identifier : rule_token<Derived, TokenSpec>
    {
        static constexpr const char* name = "<identifier>";
    };

    /// Whether or not the given token is an identifier.
    template <class Token>
    struct is_identifier : detail::is_token_impl<identifier, Token>::value
    {};

    /// Whether or not the given token is a rule token that is not an identifier.
    template <class Token>
    struct is_non_identifier_rule_token
    : std::integral_constant<bool, is_rule_token<Token>::value && !is_identifier<Token>::value>
    {};

    /// A keyword.
    ///
    /// This is a special [lex::literal_token]().
    /// If it is present in a token specification, the literal token must be present as well.
    /// It matches any literal that consists of this exact sequence of characters.
    template <char... Char>
    struct keyword : literal_token<Char...>
    {};

    /// Expands to `keyword<String[0], String[1], ...>`.
    /// It ignores all null characters.
#define FOONATHAN_LEX_KEYWORD(String) FOONATHAN_LEX_DETAIL_STRING(foonathan::lex::keyword, String)

    /// Whether or not the given token is a keyword.
    template <class Token>
    struct is_keyword : detail::is_literal_token_impl<keyword, Token>::value
    {};

    /// Whether or not the given token is a literal token that is not a keyword.
    template <class Token>
    struct is_non_keyword_literal_token
    : std::integral_constant<bool, is_literal_token<Token>::value && !is_keyword<Token>::value>
    {};

    namespace detail
    {
        template <class TokenSpec, class Identifiers, class Keywords>
        struct keyword_identifier_matcher
        {
            static_assert(Identifiers::size <= 1, "at most one identifier token is allowed");
        };

        template <class TokenSpec, class Identifier, class Keywords>
        struct keyword_identifier_matcher<TokenSpec, type_list<Identifier>, Keywords>
        {
            using keyword_matcher = literal_matcher<TokenSpec, Keywords>;

            static constexpr match_result<TokenSpec> try_match(const char* str,
                                                               const char* end) noexcept
            {
                auto identifier = Identifier::try_match(str, end);
                if (!identifier.is_success())
                    // not an identifier, so can't be a keyword
                    return identifier;

                // try to match a keyword in the identifier
                auto identifier_begin = str;
                auto identifier_end   = str + identifier.bump;
                auto keyword = literal_matcher<TokenSpec, Keywords>::try_match(identifier_begin,
                                                                               identifier_end);
                if (keyword.is_matched() && keyword.bump == identifier.bump)
                    // we've matched a keyword and it isn't a prefix but the whole string
                    return keyword;
                else
                    // didn't match keyword or it was only a prefix
                    return identifier;
            }
        };

        template <class TokenSpec, class Keywords>
        struct keyword_identifier_matcher<TokenSpec, type_list<>, Keywords>
        {
            static_assert(Keywords::size == 0, "keyword tokens require an identifier token");

            static constexpr match_result<TokenSpec> try_match(const char*, const char*) noexcept
            {
                // no identifier rule, so will never match
                return match_result<TokenSpec>{};
            }
        };
    } // namespace detail
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_IDENTIFIER_HPP_INCLUDED
