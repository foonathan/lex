// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_DETAIL_SELECT_INTEGER_HPP_INCLUDED
#define FOONATHAN_LEX_DETAIL_SELECT_INTEGER_HPP_INCLUDED

#include <climits>
#include <cstdint>
#include <type_traits>

namespace foonathan
{
namespace lex
{
    namespace detail
    {
        template <std::size_t Size, typename = void>
        struct select_integer_impl
        {
            static_assert(Size == 0u, "too high");
            using type = void;
        };

#define FOONATHAN_LEX_DETAIL_SELECT(Min, Max, Type)                                                \
    template <std::size_t Size>                                                                    \
    struct select_integer_impl<Size, std::enable_if_t<(Size >= (Min) && Size <= (Max))>>           \
    {                                                                                              \
        using type = Type;                                                                         \
    };

        FOONATHAN_LEX_DETAIL_SELECT(0, UINT_LEAST8_MAX, std::uint_least8_t)
        FOONATHAN_LEX_DETAIL_SELECT(UINT_LEAST8_MAX + 1ull, UINT_LEAST16_MAX, std::uint_least16_t)
        FOONATHAN_LEX_DETAIL_SELECT(UINT_LEAST16_MAX + 1ull, UINT_LEAST32_MAX, std::uint_least32_t)
        FOONATHAN_LEX_DETAIL_SELECT(UINT_LEAST32_MAX + 1ull, UINT_LEAST64_MAX, std::uint_least64_t)

#undef FOONATHAN_LEX_DETAIL_SELECT

        template <std::size_t MaxSize>
        using select_integer = typename select_integer_impl<MaxSize>::type;
    } // namespace detail
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_DETAIL_SELECT_INTEGER_HPP_INCLUDED
