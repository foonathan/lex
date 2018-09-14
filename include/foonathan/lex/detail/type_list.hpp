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

            //=== index_of ===//
            template <class List, typename T>
            struct index_of_impl;

            template <typename T>
            struct index_of_impl<type_list<>, T> : std::integral_constant<std::size_t, 0>
            {
            };

            template <typename... Tail, typename T>
            struct index_of_impl<type_list<T, Tail...>, T> : std::integral_constant<std::size_t, 0>
            {
            };

            template <typename Head, typename... Tail, typename T>
            struct index_of_impl<type_list<Head, Tail...>, T>
            : std::integral_constant<std::size_t, 1 + index_of_impl<type_list<Tail...>, T>::value>
            {
            };

            template <class List, typename T>
            using index_of = index_of_impl<List, T>;

            template <class List, typename T>
            using contains = std::integral_constant<bool, (index_of<List, T>::value < List::size)>;
        } // namespace detail
    }     // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_DETAIL_TYPE_LIST_HPP_INCLUDED
