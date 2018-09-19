// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_TOKEN_KIND_HPP_INCLUDED
#define FOONATHAN_LEX_TOKEN_KIND_HPP_INCLUDED

#include <cstdint>

#include <foonathan/lex/detail/select_integer.hpp>
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
        constexpr id_type<TokenSpec> get_id(eof) noexcept
        {
            return 1;
        }
    } // namespace detail

    /// Information about the kind of a token.
    template <class TokenSpec>
    class token_kind
    {
    public:
        /// \effects Creates an invalid token kind.
        constexpr token_kind() noexcept : id_(0) {}

        /// \effects Creates the EOF token kind.
        constexpr token_kind(eof) noexcept : id_(detail::get_id<TokenSpec>(eof{})) {}

        /// \effects Creates the specified token kind.
        /// \requires The token must be one of the specified tokens.
        template <class Token>
        constexpr token_kind(Token) noexcept : id_(detail::get_id<TokenSpec>(Token{}))
        {}

        /// \returns Whether or not it is invalid.
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
        auto get() const noexcept
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
            else if (is(eof{}))
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
                // TOOD: assert
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
