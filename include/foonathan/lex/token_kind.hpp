// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
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
        struct get_id_impl
        {
            static constexpr id_type<TokenSpec> get() noexcept
            {
                constexpr auto index = detail::index_of<TokenSpec, Token>::value;
                static_assert(detail::contains<TokenSpec, Token>::value,
                              "not one of the specified tokens");
                return static_cast<id_type<TokenSpec>>(index + 2);
            }
        };

        template <class TokenSpec>
        struct get_id_impl<TokenSpec, error_token>
        {
            static constexpr id_type<TokenSpec> get() noexcept
            {
                return 0;
            }
        };

        template <class TokenSpec>
        struct get_id_impl<TokenSpec, eof_token>
        {
            static constexpr id_type<TokenSpec> get() noexcept
            {
                return 1;
            }
        };

        template <class TokenSpec, class Token>
        constexpr id_type<TokenSpec> get_id() noexcept
        {
            return get_id_impl<TokenSpec, Token>::get();
        }

        template <class TokenSpec, template <typename> class Category>
        struct category_matcher
        {
            id_type<TokenSpec> id;
            bool               result = false;

            template <typename T>
            constexpr bool operator()(type_list<T>)
            {
                if (id != get_id<TokenSpec, T>())
                    return true;
                else
                {
                    result = Category<T>::value;
                    return false;
                }
            }
        };
    } // namespace detail

    /// Information about the kind of a token.
    template <class TokenSpec>
    class token_kind
    {
    public:
        /// \effects Creates it from the integral id.
        static constexpr token_kind from_id(std::size_t id) noexcept
        {
            return token_kind(0, static_cast<detail::id_type<TokenSpec>>(id));
        }

        /// \effects Creates it from an incomplete token type.
        /// But otherwise behaves like the constructor.
        template <class Token>
        static constexpr token_kind of() noexcept
        {
            return token_kind(0, detail::get_id<TokenSpec, Token>());
        }

        /// \effects Creates an error token kind.
        /// \group ctor_error
        constexpr token_kind() noexcept : token_kind(error_token{}) {}
        /// \group ctor_error
        constexpr token_kind(error_token) noexcept : id_(detail::get_id<TokenSpec, error_token>())
        {}

        /// \effects Creates the EOF token kind.
        constexpr token_kind(eof_token) noexcept : id_(detail::get_id<TokenSpec, eof_token>()) {}

        /// \effects Creates the specified token kind.
        /// \requires The token must be one of the specified tokens.
        template <class Token>
        constexpr token_kind(Token) noexcept : id_(detail::get_id<TokenSpec, Token>())
        {}

        /// \returns Whether or not it is the error token.
        explicit constexpr operator bool() const noexcept
        {
            return id_ != 0;
        }

        /// \returns Whether or not it is the specified token kind.
        template <class Token>
        constexpr bool is(Token = {}) const noexcept
        {
            return id_ == detail::get_id<TokenSpec, Token>();
        }

        template <template <typename> class Category>
        constexpr bool is_category() const noexcept
        {
            detail::category_matcher<TokenSpec, Category> matcher{id_, false};
            detail::for_each(TokenSpec{}, matcher);
            return matcher.result;
        }

        /// \returns The underlying integer value of the token.
        constexpr detail::id_type<TokenSpec> get() const noexcept
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
                    if (!this->is(type{}))
                        return true;

                    result = type::name;
                    return false;
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
