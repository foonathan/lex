// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_ASCII_HPP_INCLUDED
#define FOONATHAN_LEX_ASCII_HPP_INCLUDED

#include <climits>
#include <foonathan/lex/rule_token.hpp>

namespace foonathan
{
namespace lex
{
    /// Functions to check the category of ASCII characters.
    ///
    /// They are subdivided into the following categories:
    /// * `ascii` is `control`, `blank`, `newline`, `other_space`, `digit`, `lower`, `upper`, or
    /// `punct`.
    /// * `space` is `blank`, `newline`, or `other_space`.
    /// * `alpha` is `lower` or `upper`.
    /// * `alnum` is `lower`, `upper`, or `digit`.
    /// * `graph` is `lower`, `upper`, `digit`, or `punct`
    /// * `print` is `lower`, `upper`, `digit`, `punct` or `' '`.
    namespace ascii
    {
        /// The type of all functions in this namespace.
        using predicate = bool (*)(char);

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

        /// \returns Whether or not the character is an ASCII character.
        constexpr bool is_ascii(char c) noexcept
        {
            static_assert(0x7F <= CHAR_MAX, "???");
            return detail::is_ascii_impl(std::is_signed<char>{}, c);
        }

        /// \returns Whether or not the character is an ASCII control character other than space
        /// characters, i.e. a character in the range `0x00` to `0x08`, or `0x0E` to `0x1F` or
        /// `0x7F`.
        constexpr bool is_control(char c) noexcept
        {
            return (c >= 0x00 && c <= 0x08) || (c >= 0x0E && c <= 0x1F) || c == 0x7F;
        }

        /// \returns Whether or not the character is an ASCII blank character, i.e. space ' ' or tab
        /// `\t`.
        constexpr bool is_blank(char c) noexcept
        {
            return c == ' ' || c == '\t';
        }

        /// \returns Whether or not the character is an ASCII end of line character, i.e. newline
        /// `\n` or carriage return `\r`.
        constexpr bool is_newline(char c) noexcept
        {
            return c == '\n' || c == '\r';
        }

        /// \returns Whether or not the character is some other ASCII space,
        /// i.e. form feed `\f` or vertical tab `\v`.
        constexpr bool is_other_space(char c) noexcept
        {
            return c == '\f' || c == '\v';
        }

        /// \returns Whether or not the character is an ASCII whitespace character,
        /// i.e. `is_blank(c) || is_newline(c) || is_other_space(c)`.
        constexpr bool is_space(char c) noexcept
        {
            return is_blank(c) || is_newline(c) || is_other_space(c);
        }

        /// \returns Whether or not the character is an ASCII digit, `0` to `9`.
        constexpr bool is_digit(char c) noexcept
        {
            // guaranteed to be stored contiguously
            return c >= '0' && c <= '9';
        }

        /// \returns Whether or not the character is a lower-case ASCII character, `a` to `z`.
        constexpr bool is_lower(char c) noexcept
        {
            static_assert('a' + 25 == 'z', "machine is not using ASCII...?");
            return c >= 'a' && c <= 'z';
        }

        /// \returns Whether or not the character is an upper-case ASCII character, `A` to `Z`.
        constexpr bool is_upper(char c) noexcept
        {
            static_assert('A' + 25 == 'Z', "machine is not using ASCII...?");
            return c >= 'A' && c <= 'Z';
        }

        /// \returns `is_lower(c) || is_upper(c)`.
        constexpr bool is_alpha(char c) noexcept
        {
            return is_lower(c) || is_upper(c);
        }

        /// \returns `is_alpha(c) || is_digit(c)`.
        constexpr bool is_alnum(char c) noexcept
        {
            return is_alpha(c) || is_digit(c);
        }

        /// \returns Whether or not the character is an ASCII punctuation character,
        /// i.e. one of ``!"#$%&'()*+,-./:;<=>?@[\]^_`{|}~``.
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

        /// \returns `is_alnum(c) || is_punct(c)`.
        constexpr bool is_graph(char c) noexcept
        {
            return is_alnum(c) || is_punct(c);
        }

        /// \returns `is_graph(c) || c == ' '`.
        constexpr bool is_print(char c) noexcept
        {
            return c == ' ' || is_graph(c);
        }
    } // namespace ascii
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_ASCII_HPP_INCLUDED
