// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_ASCII_HPP_INCLUDED
#define FOONATHAN_LEX_ASCII_HPP_INCLUDED

#include <climits>
#include <type_traits>

namespace foonathan
{
namespace lex
{
    namespace ascii
    {
        namespace detail
        {
            template <typename T>
            constexpr bool is_ascii_impl(std::true_type /* signed */, T c) noexcept
            {
                // all positive values are ASCII
                return c >= 0;
            }

            template <typename T>
            constexpr bool is_ascii_impl(std::false_type /* unsigned */, T c) noexcept
            {
                // all values less than or equal to 0x7F  are ASCII
                return c <= 0x7F;
            }
        } // namespace detail

        constexpr bool is_ascii(char c) noexcept
        {
            static_assert(0x7F <= CHAR_MAX, "???");
            return detail::is_ascii_impl(std::is_signed<char>{}, c);
        }

        constexpr bool is_control(char c) noexcept
        {
            return (c >= 0x00 && c <= 0x08) || (c >= 0x0E && c <= 0x1F) || c == 0x7F;
        }

        constexpr bool is_blank(char c) noexcept
        {
            return c == ' ' || c == '\t';
        }

        constexpr bool is_newline(char c) noexcept
        {
            return c == '\n' || c == '\r';
        }

        constexpr bool is_other_space(char c) noexcept
        {
            return c == '\f' || c == '\v';
        }

        constexpr bool is_space(char c) noexcept
        {
            return is_blank(c) || is_newline(c) || is_other_space(c);
        }

        constexpr bool is_digit(char c) noexcept
        {
            // guaranteed to be stored contiguously
            return c >= '0' && c <= '9';
        }

        constexpr bool is_lower(char c) noexcept
        {
            static_assert('a' + 25 == 'z', "machine is not using ASCII...?");
            return c >= 'a' && c <= 'z';
        }

        constexpr bool is_upper(char c) noexcept
        {
            static_assert('A' + 25 == 'Z', "machine is not using ASCII...?");
            return c >= 'A' && c <= 'Z';
        }

        constexpr bool is_alpha(char c) noexcept
        {
            return is_lower(c) || is_upper(c);
        }

        constexpr bool is_alnum(char c) noexcept
        {
            return is_alpha(c) || is_digit(c);
        }

        constexpr bool is_punct(char c) noexcept
        {
            static_assert('!' + 1 == '"', "machine is not using ASCII...?");
            auto first_punct = c >= '!' && c <= '/';

            static_assert(':' + 1 == ';', "machine is not using ASCII...?");
            auto second_punct = c >= ':' && c <= '@';

            static_assert('[' + 1 == '\\', "machine is not using ASCII...?");
            auto third_punct = c >= '[' && c <= '`';

            static_assert('{' + 1 == '|', "machine is not using ASCII...?");
            auto fourth_punct = c >= '{' && c <= '~';

            return first_punct || second_punct || third_punct || fourth_punct;
        }

        constexpr bool is_graph(char c) noexcept
        {
            return is_alnum(c) || is_punct(c);
        }

        constexpr bool is_print(char c) noexcept
        {
            return c == ' ' || is_graph(c);
        }
    } // namespace ascii
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_ASCII_HPP_INCLUDED
