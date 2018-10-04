// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_RULE_TOKEN_HPP_INCLUDED
#define FOONATHAN_LEX_RULE_TOKEN_HPP_INCLUDED

#include <foonathan/lex/match_result.hpp>
#include <foonathan/lex/token_spec.hpp>

namespace foonathan
{
namespace lex
{
    /// A token that has no associated parsing rule.
    ///
    /// It can only be created by some other [lex::rule_token]().
    struct null_token : detail::base_token
    {};

    /// Whether or not the given token is a null token.
    template <class Token>
    struct is_null_token : std::is_base_of<null_token, Token>
    {};

    /// A token that follows a complex parsing rule.
    ///
    /// It must provide a function `static match_result try_match(const char* cur, const char*
    /// end) noexcept;`. It is invoked with a pointer to the current character and to the end.
    /// There will always be at least one character.
    ///
    /// This function tries to parse a token and reports success or failure.
    /// If it didn't match anything, the next rule is tried.
    /// If it was an error, an error token is created.
    /// If it was a success, the correct token is created.
    ///
    /// It may create tokens of multiple other kinds if the parsing is related.
    /// The other tokens must then be [lex::null_token]() because they have no rules on their
    /// own. \notes The rules are tried in an arbitrary order so code should not depend on any
    /// particular ordering.
    template <class Derived, class TokenSpec>
    struct rule_token : detail::base_token
    {
        using spec         = TokenSpec;
        using token_kind   = lex::token_kind<TokenSpec>;
        using match_result = lex::match_result<TokenSpec>;

        /// \returns An unmatched result.
        static constexpr match_result unmatched() noexcept
        {
            return match_result::unmatched();
        }

        /// \returns An error result consuming the given number of characters.
        static constexpr match_result error(std::size_t bump) noexcept
        {
            return match_result::error(bump);
        }

        /// \returns A matched result creating the `Derived` token and consuming the given
        /// number of characters.
        template <typename Integer>
        static constexpr match_result success(Integer bump) noexcept
        {
            return match_result::success(token_kind(Derived{}), static_cast<std::size_t>(bump));
        }

        /// \returns A matched result creating some other null token and consuming the given
        /// number of characters.
        template <class Token, typename Integer>
        static constexpr match_result success(Integer bump) noexcept
        {
            return match_result::success(token_kind(Token{}), static_cast<std::size_t>(bump));
        }

        constexpr rule_token()
        {
            static_assert(std::is_base_of<rule_token, Derived>::value, "invalid type for derived");
        }
    };

    /// Whether or not the given token is a rule token.
    template <class Token>
    struct is_rule_token : detail::is_token_impl<rule_token, Token>::value
    {};
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_RULE_TOKEN_HPP_INCLUDED
