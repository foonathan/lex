// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_TOKEN_KIND_HPP_INCLUDED
#define FOONATHAN_LEX_TOKEN_KIND_HPP_INCLUDED

#include <boost/mp11/algorithm.hpp>

#include <foonathan/lex/detail/assert.hpp>
#include <foonathan/lex/detail/select_integer.hpp>
#include <foonathan/lex/token_spec.hpp>

namespace foonathan
{
namespace lex
{
    namespace token_kind_detail
    {
        template <class TokenSpec>
        using id_type
            = detail::select_integer<boost::mp11::mp_size<typename TokenSpec::list>::value>;

        template <class TokenSpec, class Token>
        constexpr id_type<TokenSpec> get_id() noexcept
        {
            static_assert(boost::mp11::mp_contains<typename TokenSpec::list, Token>::value,
                          "not one of the specified tokens");
            constexpr auto index = boost::mp11::mp_find<typename TokenSpec::list, Token>::value;
            return static_cast<id_type<TokenSpec>>(index);
        }
    } // namespace token_kind_detail

    template <class TokenSpec>
    class token_kind
    {
    public:
        static constexpr token_kind from_id(std::size_t id) noexcept
        {
            return token_kind(0, static_cast<token_kind_detail::id_type<TokenSpec>>(id));
        }

        template <class Token>
        static constexpr token_kind of() noexcept
        {
            return token_kind(0, token_kind_detail::get_id<TokenSpec, Token>());
        }

        constexpr token_kind() noexcept : token_kind(error_token{}) {}

        template <class Token>
        constexpr token_kind(Token) noexcept : id_(token_kind_detail::get_id<TokenSpec, Token>())
        {}

        explicit constexpr operator bool() const noexcept
        {
            return id_ != token_kind_detail::get_id<TokenSpec, error_token>();
        }

        template <class Token>
        constexpr bool is(Token = {}) const noexcept
        {
            return id_ == token_kind_detail::get_id<TokenSpec, Token>();
        }

        template <template <typename> class Category>
        constexpr bool is_category() const noexcept
        {
            return is_category_impl<Category>(typename TokenSpec::list{});
        }

        constexpr token_kind_detail::id_type<TokenSpec> get() const noexcept
        {
            return id_;
        }

        constexpr const char* name() const noexcept
        {
            return name_impl(typename TokenSpec::list{});
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
        explicit constexpr token_kind(int, token_kind_detail::id_type<TokenSpec> id) noexcept
        : id_(id)
        {}

        template <template <typename> class Category, typename... Tokens>
        constexpr bool is_category_impl(boost::mp11::mp_list<Tokens...>) const noexcept
        {
            bool result  = false;
            bool dummy[] = {(is<Tokens>() && (result = Category<Tokens>::value, true))..., true};
            (void)dummy;
            return result;
        }

        template <typename... Tokens>
        constexpr const char* name_impl(boost::mp11::mp_list<Tokens...>) const noexcept
        {
            const char* result  = nullptr;
            bool        dummy[] = {(is<Tokens>() && (result = Tokens::name, true))..., true};
            (void)dummy;
            FOONATHAN_LEX_ASSERT(result);
            return result;
        }

        token_kind_detail::id_type<TokenSpec> id_;
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
