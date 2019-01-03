// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
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

            using list = type_list<Types...>;
        };

        template <typename T>
        struct type_list<T>
        {
            static constexpr auto size = 1;
            using type                 = T;

            template <typename>
            using type_or = T;

            using list = type_list<T>;
        };

        template <>
        struct type_list<>
        {
            static constexpr auto size = 0;

            template <typename T>
            using type_or = T;

            using list = type_list<>;
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
        using concat = typename cat_impl<typename List::list, Ts...>::type;

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
        using index_of = index_of_impl<typename List::list, T>;

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
        using keep_if = typename filter_impl<typename List::list, Predicate>::positive;

        template <class List, template <typename> class Predicate>
        using remove_if = typename filter_impl<typename List::list, Predicate>::negative;

        //=== all_of/none_of/any_of ===//
        template <bool... Bools>
        struct bool_list
        {};

        template <bool... Bools>
        using all_true
            = std::is_same<bool_list<Bools..., true>, bool_list<true, Bools...>>; // neat trick
        template <bool... Bools>
        using none_true = all_true<(!Bools)...>; // none are true if all are false
        template <bool... Bools>
        using any_true
            = std::integral_constant<bool, !none_true<Bools...>::value>; // if not none_true, then
                                                                         // something must be true

        template <class List, template <typename T> class Pred>
        struct all_of_impl;
        template <typename... Types, template <typename T> class Pred>
        struct all_of_impl<type_list<Types...>, Pred>
        {
            using type = all_true<Pred<Types>::value...>;
        };

        template <class List, template <typename T> class Pred>
        struct none_of_impl;
        template <typename... Types, template <typename T> class Pred>
        struct none_of_impl<type_list<Types...>, Pred>
        {
            using type = none_true<Pred<Types>::value...>;
        };

        template <class List, template <typename T> class Pred>
        struct any_of_impl;
        template <typename... Types, template <typename T> class Pred>
        struct any_of_impl<type_list<Types...>, Pred>
        {
            using type = any_true<Pred<Types>::value...>;
        };

        template <class List, template <typename T> class Pred>
        using all_of = typename all_of_impl<typename List::list, Pred>::type;

        template <class List, template <typename T> class Pred>
        using none_of = typename none_of_impl<typename List::list, Pred>::type;

        template <class List, template <typename T> class Pred>
        using any_of = typename any_of_impl<typename List::list, Pred>::type;

        //=== for_each ===//
        template <class List>
        struct for_each_impl;

        template <>
        struct for_each_impl<type_list<>>
        {
            template <typename Func>
            static constexpr void apply(const Func&) noexcept
            {}
        };

        template <typename Head, typename... Tail>
        struct for_each_impl<type_list<Head, Tail...>>
        {
            template <typename Func>
            static constexpr void apply(Func&& f) noexcept
            {
                if (!f(type_list<Head>{}))
                    return;
                for_each_impl<type_list<Tail...>>::apply(f);
            }
        };

        template <class List, typename Func>
        constexpr void for_each(List, Func&& f) noexcept
        {
            for_each_impl<typename List::list>::apply(f);
        }
    } // namespace detail
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_DETAIL_TYPE_LIST_HPP_INCLUDED
