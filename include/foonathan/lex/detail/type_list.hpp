// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_DETAIL_TYPE_LIST_HPP_INCLUDED
#define FOONATHAN_LEX_DETAIL_TYPE_LIST_HPP_INCLUDED

#include <type_traits>

namespace foonathan
{
namespace lex
{
    namespace detail
    {
        template <typename... Types>
        struct type_list
        {
            static constexpr auto size = sizeof...(Types);
        };

        template <typename T>
        struct type_list<T>
        {
            static constexpr auto size = 1;
            using type                 = T;
        };

        //=== cat ===//
        template <class List, typename... Ts>
        struct cat_impl;

        template <typename... Ts>
        struct cat_impl<type_list<Ts...>>
        {
            using type = type_list<Ts...>;
        };

        template <typename... Ts, typename T, typename... Tail>
        struct cat_impl<type_list<Ts...>, T, Tail...>
        {
            using type = typename cat_impl<type_list<Ts..., T>, Tail...>::type;
        };

        template <typename... Ts, typename... OtherTs, typename... Tail>
        struct cat_impl<type_list<Ts...>, type_list<OtherTs...>, Tail...>
        {
            using type = typename cat_impl<type_list<Ts..., OtherTs...>, Tail...>::type;
        };

        template <class List, typename... Ts>
        using concat = typename cat_impl<List, Ts...>::type;

        //=== index_of ===//
        template <class List, typename T>
        struct index_of_impl;

        template <typename T>
        struct index_of_impl<type_list<>, T> : std::integral_constant<std::size_t, 0>
        {};

        template <typename... Tail, typename T>
        struct index_of_impl<type_list<T, Tail...>, T> : std::integral_constant<std::size_t, 0>
        {};

        template <typename Head, typename... Tail, typename T>
        struct index_of_impl<type_list<Head, Tail...>, T>
        : std::integral_constant<std::size_t, 1 + index_of_impl<type_list<Tail...>, T>::value>
        {};

        template <class List, typename T>
        using index_of = index_of_impl<List, T>;

        template <class List, typename T>
        using contains = std::integral_constant<bool, (index_of<List, T>::value < List::size)>;

        //=== filter ===//
        template <class List, template <typename> class Predicate>
        struct filter_impl;

        template <template <typename> class Predicate>
        struct filter_impl<type_list<>, Predicate>
        {
            using positive = type_list<>;
            using negative = type_list<>;
        };

        template <typename Head, typename... Tail, template <typename> class Predicate>
        struct filter_impl<type_list<Head, Tail...>, Predicate>
        {
            using tail          = filter_impl<type_list<Tail...>, Predicate>;
            using tail_positive = typename tail::positive;
            using tail_negative = typename tail::negative;

            using head_positive = Predicate<Head>;

            using positive
                = std::conditional_t<head_positive::value, concat<type_list<Head>, tail_positive>,
                                     tail_positive>;
            using negative = std::conditional_t<head_positive::value, tail_negative,
                                                concat<type_list<Head>, tail_negative>>;
        };

        template <class List, template <typename> class Predicate>
        using keep_if = typename filter_impl<List, Predicate>::positive;

        template <class List, template <typename> class Predicate>
        using remove_if = typename filter_impl<List, Predicate>::negative;

        //=== for_each ===//
        template <class List, typename Func>
        struct for_each_impl;

        template <typename Func>
        struct for_each_impl<type_list<>, Func>
        {
            static constexpr void apply(const Func&) noexcept {}
        };

        template <typename Head, typename... Tail, typename Func>
        struct for_each_impl<type_list<Head, Tail...>, Func>
        {
            static constexpr void apply(const Func& f) noexcept
            {
                if (!f(type_list<Head>{}))
                    return;
                for_each_impl<type_list<Tail...>, Func>::apply(f);
            }
        };

        template <class List, typename Func>
        constexpr void for_each(List, Func f) noexcept
        {
            for_each_impl<List, Func>::apply(f);
        }
    } // namespace detail
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_DETAIL_TYPE_LIST_HPP_INCLUDED
