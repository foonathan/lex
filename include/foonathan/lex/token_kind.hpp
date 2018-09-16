// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_TOKEN_KIND_HPP_INCLUDED
#define FOONATHAN_LEX_TOKEN_KIND_HPP_INCLUDED

#include <cstdint>

#include <foonathan/lex/detail/type_list.hpp>

namespace foonathan
{
namespace lex
{
    template <class... Tokens>
    using token_spec = detail::type_list<Tokens...>;

    /// Tag type to mark the EOF token.
    /// It is generated at the very end.
    struct eof
    {};

    /// Information about the kind of a token.
    template <class TokenSpec>
    class token_kind
    {
    public:
        /// \effects Creates an invalid token kind.
        constexpr token_kind() noexcept : id_(0) {}

        /// \effects Creates the EOF token kind.
        constexpr token_kind(eof) noexcept : id_(1) {}

        /// \effects Creates the specified token kind.
        /// \requires The token must be one of the specified tokens.
        template <class Token>
        constexpr token_kind(Token) noexcept : id_(get_id<Token>())
        {}

        /// \returns Whether or not it is invalid.
        explicit constexpr operator bool() const noexcept
        {
            return id_ != 0;
        }

        /// \returns Whether or not it is eof.
        constexpr bool is(eof = {}) const noexcept
        {
            return id_ == 1;
        }

        /// \returns Whether or not it is the specified token kind.
        template <class Token>
        constexpr bool is(Token = {}) const noexcept
        {
            return id_ == get_id<Token>();
        }

        friend constexpr bool operator==(token_kind lhs, token_kind rhs) noexcept
        {
            return lhs.id_ == rhs.id_;
        }
        friend constexpr bool operator!=(token_kind lhs, token_kind rhs) noexcept
        {
            return !(lhs == rhs);
        }

        template <class Token>
        friend constexpr bool operator==(token_kind lhs, Token rhs) noexcept
        {
            return lhs.is(rhs);
        }
        template <class Token>
        friend constexpr bool operator==(Token lhs, token_kind rhs) noexcept
        {
            return rhs == lhs;
        }
        template <class Token>
        friend constexpr bool operator!=(token_kind lhs, Token rhs) noexcept
        {
            return !(lhs == rhs);
        }
        template <class Token>
        friend constexpr bool operator!=(Token lhs, token_kind rhs) noexcept
        {
            return !(lhs == rhs);
        }

    private:
        template <class Token>
        static constexpr std::uint32_t get_id() noexcept
        {
            constexpr auto index = detail::index_of<TokenSpec, Token>::value;
            static_assert(detail::contains<TokenSpec, Token>::value,
                          "not one of the specified tokens");
            // id 0: invalid
            // id 1: EOF
            return static_cast<std::uint32_t>(index + 2);
        }

        std::uint32_t id_;
    };

    namespace detail
    {
        template <template <typename...> class Kind, class Token>
        struct is_token_impl
        {
            template <typename... Args>
            static std::true_type  test(const Kind<Token, Args...>&);
            static std::false_type test(...);

            using value = decltype(test(std::declval<Token>()));
        };
    } // namespace detail
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_TOKEN_KIND_HPP_INCLUDED
