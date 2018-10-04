// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_TOKEN_KIND_HPP_INCLUDED
#define FOONATHAN_LEX_TOKEN_KIND_HPP_INCLUDED

#include <foonathan/lex/detail/select_integer.hpp>
#include <foonathan/lex/token_spec.hpp>

namespace foonathan
{
namespace lex
{
    namespace detail
    {
        // id 0: invalid
        // id 1: EOF
        template <class TokenSpec>
        using id_type = detail::select_integer<TokenSpec::size + 2>;

        template <class TokenSpec, class Token>
        constexpr id_type<TokenSpec> get_id(Token) noexcept
        {
            constexpr auto index = detail::index_of<TokenSpec, Token>::value;
            static_assert(detail::contains<TokenSpec, Token>::value,
                          "not one of the specified tokens");
            return static_cast<id_type<TokenSpec>>(index + 2);
        }

        template <class TokenSpec>
        constexpr id_type<TokenSpec> get_id(error_token) noexcept
        {
            return 0;
        }

        template <class TokenSpec>
        constexpr id_type<TokenSpec> get_id(eof_token) noexcept
        {
            return 1;
        }
    } // namespace detail

    /// Information about the kind of a token.
    template <class TokenSpec>
    class token_kind
    {
    public:
        /// \effects Creates it from the integral id.
        static constexpr token_kind from_id(detail::id_type<TokenSpec> id) noexcept
        {
            return token_kind(0, id);
        }

        /// \effects Creates an error token kind.
        /// \group ctor_error
        constexpr token_kind() noexcept : token_kind(error_token{}) {}
        /// \group ctor_error
        constexpr token_kind(error_token) noexcept : id_(detail::get_id<TokenSpec>(error_token{}))
        {}

        /// \effects Creates the EOF token kind.
        constexpr token_kind(eof_token) noexcept : id_(detail::get_id<TokenSpec>(eof_token{})) {}

        /// \effects Creates the specified token kind.
        /// \requires The token must be one of the specified tokens.
        template <class Token>
        constexpr token_kind(Token) noexcept : id_(detail::get_id<TokenSpec>(Token{}))
        {}

        /// \returns Whether or not it is the error token.
        explicit constexpr operator bool() const noexcept
        {
            return id_ != 0;
        }

        /// \returns Whether or not it is the specified token kind.
        template <class Token>
        constexpr bool is(Token token = {}) const noexcept
        {
            return id_ == detail::get_id<TokenSpec>(token);
        }

        /// \returns The underlying integer value of the token.
        constexpr auto get() const noexcept
        {
            return id_;
        }

        /// \returns The name of the token.
        /// If this function is used, all rule tokens must have a `static constexpr const char*
        /// name;` member. This is returned as the name. For tokens that inherit from a class other
        /// than [lex::rule_token]() it is automatically provided, but can of course be overriden by
        /// hiding the declaration.
        constexpr const char* name() const noexcept
        {
            if (!*this)
                return "<error>";
            else if (is(eof_token{}))
                return "<eof>";
            else
            {
                const char* result = nullptr;
                detail::for_each(TokenSpec{}, [&](auto tag) {
                    using type = typename decltype(tag)::type;
                    if (!is(type{}))
                        return true;
                    else
                    {
                        result = type::name;
                        return false;
                    }
                });
                FOONATHAN_LEX_ASSERT(result);
                return result;
            }
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
        explicit constexpr token_kind(int, detail::id_type<TokenSpec> id) noexcept : id_(id) {}

        detail::id_type<TokenSpec> id_;
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
