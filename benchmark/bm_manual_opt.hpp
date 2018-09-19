// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_BM_MANUAL_OPT_HPP_INCLUDED
#define FOONATHAN_LEX_BM_MANUAL_OPT_HPP_INCLUDED

#include <foonathan/lex/ascii.hpp>
#include <foonathan/lex/spelling.hpp>

namespace manual_opt_ns
{
template <unsigned N>
bool starts_with(const char* str, const char* end, const char (&token)[N])
{
    auto remaining = unsigned(end - str);
    if (remaining < N - 1)
        return false;
    else
        return std::strncmp(str, token, N - 1) == 0;
}

foonathan::lex::token_spelling bump(const char*& str, unsigned n)
{
    auto spelling = foonathan::lex::token_spelling(str, n);
    str += n;
    return spelling;
}
} // namespace manual_opt_ns

void manual_opt(const char* str, const char* end, void (*f)(int, foonathan::lex::token_spelling))
{
    using namespace manual_opt_ns;
    namespace lex = foonathan::lex;

    while (str != end)
    {
        switch (*str)
        {
        case '.':
            if (starts_with(str, end, "..."))
                f(0, bump(str, 3));
            else
                f(1, bump(str, 1));
            break;

        case '+':
            if (str + 1 != end)
                switch (str[1])
                {
                case '=':
                    f(2, bump(str, 2));
                    break;
                case '+':
                    f(3, bump(str, 2));
                    break;
                default:
                    f(4, bump(str, 1));
                    break;
                }
            else
                f(4, bump(str, 1));
            break;

        case '-':
            if (str + 1 != end)
                switch (str[1])
                {
                case '>':
                    if (str + 2 != end && str[2] == '*')
                        f(5, bump(str, 3));
                    else
                        f(6, bump(str, 2));
                case '-':
                    f(7, bump(str, 2));
                    break;
                case '=':
                    f(8, bump(str, 2));
                    break;

                default:
                    f(9, bump(str, 1));
                }
            else
                f(9, bump(str, 1));
            break;

        case '~':
            f(10, bump(str, 1));
            break;

        case ' ':
        case '\t':
        case '\n':
        case '\r':
        case '\f':
        case '\v': // whitespace
        {
            auto begin = str++;
            while (str != end && lex::ascii::is_space(*str))
                ++str;
            f(11, lex::token_spelling(begin, static_cast<std::size_t>(str - begin)));
            break;
        }

        default:
            ++str;
            break;
        }
    }
}

#endif // FOONATHAN_LEX_BM_MANUAL_OPT_HPP_INCLUDED
