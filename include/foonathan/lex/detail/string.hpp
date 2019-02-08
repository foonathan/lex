// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_DETAIL_STRING_HPP_INCLUDED
#define FOONATHAN_LEX_DETAIL_STRING_HPP_INCLUDED

#include <type_traits>

#if defined(__GNUC__)
namespace foonathan
{
namespace lex
{
    namespace detail
    {
        template <char... Chars>
        struct string_literal
        {
            template <template <char...> class String>
            using convert_to = String<Chars...>;
        };

        template <template <char...> class String, class Literal>
        using convert_literal_to = typename Literal::template convert_to<String>;
    } // namespace detail
} // namespace lex
} // namespace foonathan

#    ifdef __clang__
#        pragma GCC diagnostic push
#        pragma GCC diagnostic ignored "-Wgnu-string-literal-operator-template"
#    endif

template <typename Char, Char... Chars>
constexpr foonathan::lex::detail::string_literal<Chars...> operator""_foonathan_lex_string_udl()
{
    return {};
}

#    define FOONATHAN_LEX_DETAIL_STRING(String, Chars)                                             \
        foonathan::lex::detail::convert_literal_to<String,                                         \
                                                   decltype(Chars##_foonathan_lex_string_udl)>

#    ifdef __clang__
#        pragma GCC diagnostic pop
#    endif

#else
namespace foonathan
{
namespace lex
{
    namespace detail
    {
        //=== string_cat ===//
        template <typename Lhs, typename Rhs>
        struct string_cat_impl;

        template <template <char...> class String, char... Lhs, char... Rhs>
        struct string_cat_impl<String<Lhs...>, String<Rhs...>>
        {
            using type = String<Lhs..., Rhs...>;
        };

        template <typename Lhs, typename Rhs>
        using string_cat = typename string_cat_impl<Lhs, Rhs>::type;

        //=== check_string_size ===//
        template <typename T, std::size_t Size, std::size_t MaxSize>
        struct check_string_size
        {
            static_assert(Size <= MaxSize, "string out of range");
            using type = T;
        };
    } // namespace detail
} // namespace lex
} // namespace foonathan

// expands to String<Chars[I]> if that is not null
// String<> otherwise
//
// the range check inside String<Chars[I]> prevents reading out-of-bound string literal if it is
// instantiated
#    define FOONATHAN_LEX_DETAIL_STRING_TAKE(String, Chars, I)                                     \
        std::conditional_t<(I < sizeof(Chars) && Chars[I] != 0),                                   \
                           String<(I >= sizeof(Chars)) ? 0 : Chars[I]>, String<>>

// recursively split the string in two
#    define FOONATHAN_LEX_DETAIL_STRING2(String, Chars, I)                                         \
        foonathan::lex::detail::string_cat<FOONATHAN_LEX_DETAIL_STRING_TAKE(String, Chars, I),     \
                                           FOONATHAN_LEX_DETAIL_STRING_TAKE(String, Chars, I + 1)>
#    define FOONATHAN_LEX_DETAIL_STRING4(String, Chars, I)                                         \
        foonathan::lex::detail::string_cat<FOONATHAN_LEX_DETAIL_STRING2(String, Chars, I),         \
                                           FOONATHAN_LEX_DETAIL_STRING2(String, Chars, I + 2)>
#    define FOONATHAN_LEX_DETAIL_STRING8(String, Chars, I)                                         \
        foonathan::lex::detail::string_cat<FOONATHAN_LEX_DETAIL_STRING4(String, Chars, I),         \
                                           FOONATHAN_LEX_DETAIL_STRING4(String, Chars, I + 4)>
#    define FOONATHAN_LEX_DETAIL_STRING16(String, Chars, I)                                        \
        foonathan::lex::detail::string_cat<FOONATHAN_LEX_DETAIL_STRING8(String, Chars, I),         \
                                           FOONATHAN_LEX_DETAIL_STRING8(String, Chars, I + 8)>
#    define FOONATHAN_LEX_DETAIL_STRING32(String, Chars, I)                                        \
        foonathan::lex::detail::string_cat<FOONATHAN_LEX_DETAIL_STRING16(String, Chars, I),        \
                                           FOONATHAN_LEX_DETAIL_STRING16(String, Chars, I + 16)>
#    define FOONATHAN_LEX_DETAIL_STRING64(String, Chars, I)                                        \
        foonathan::lex::detail::string_cat<FOONATHAN_LEX_DETAIL_STRING32(String, Chars, I),        \
                                           FOONATHAN_LEX_DETAIL_STRING32(String, Chars, I + 32)>
#    define FOONATHAN_LEX_DETAIL_STRING128(String, Chars, I)                                       \
        foonathan::lex::detail::string_cat<FOONATHAN_LEX_DETAIL_STRING64(String, Chars, I),        \
                                           FOONATHAN_LEX_DETAIL_STRING64(String, Chars, I + 64)>
#    define FOONATHAN_LEX_DETAIL_STRING256(String, Chars, I)                                       \
        foonathan::lex::detail::string_cat<FOONATHAN_LEX_DETAIL_STRING128(String, Chars, I),       \
                                           FOONATHAN_LEX_DETAIL_STRING128(String, Chars, I + 128)>

// expands to String<Chars[0], Chars[1], ...>
// does not include embedded null characters
#    define FOONATHAN_LEX_DETAIL_STRING(String, Chars)                                             \
        foonathan::lex::detail::check_string_size<                                                 \
            FOONATHAN_LEX_DETAIL_STRING256(String, Chars, 0), sizeof(Chars) - 1, 256>::type
#endif

#endif // FOONATHAN_LEX_DETAIL_STRING_HPP_INCLUDED
