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

        template <class TokenSpec>
        class token_kind
        {
        public:
            constexpr token_kind() noexcept : id_(0) {}

            template <class Token>
            explicit constexpr token_kind(Token) noexcept : id_(get_id<Token>())
            {
            }

            explicit constexpr operator bool() const noexcept
            {
                return id_ != 0;
            }

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
                return static_cast<std::uint32_t>(index + 1);
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
    }     // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_TOKEN_KIND_HPP_INCLUDED
