// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_BM_MANUAL_HPP_INCLUDED
#define FOONATHAN_LEX_BM_MANUAL_HPP_INCLUDED

#include <cstring>
#include <foonathan/lex/ascii.hpp>
#include <foonathan/lex/spelling.hpp>

namespace manual_ns
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
} // namespace manual_ns

void manual(const char* str, const char* end, void (*f)(int, foonathan::lex::token_spelling))
{
    using namespace manual_ns;
    namespace lex = foonathan::lex;

    while (str != end)
    {
        // simple literal tokens
        if (starts_with(str, end, "..."))
            f(0, bump(str, 3));
        else if (starts_with(str, end, "."))
            f(1, bump(str, 1));
        else if (starts_with(str, end, "+="))
            f(2, bump(str, 2));
        else if (starts_with(str, end, "++"))
            f(3, bump(str, 2));
        else if (starts_with(str, end, "+"))
            f(4, bump(str, 1));
        else if (starts_with(str, end, "->*"))
            f(5, bump(str, 3));
        else if (starts_with(str, end, "->"))
            f(6, bump(str, 2));
        else if (starts_with(str, end, "--"))
            f(7, bump(str, 2));
        else if (starts_with(str, end, "-="))
            f(8, bump(str, 2));
        else if (starts_with(str, end, "-"))
            f(9, bump(str, 1));
        else if (starts_with(str, end, "~"))
            f(10, bump(str, 1));
        // whitespace
        else if (lex::ascii::is_space(*str))
        {
            auto begin = str++;
            while (str != end && lex::ascii::is_space(*str))
                ++str;
            f(11, lex::token_spelling(begin, static_cast<std::size_t>(str - begin)));
        }
        // error
        else
            ++str;
    }
}

#endif // FOONATHAN_LEX_BM_MANUAL_HPP_INCLUDED
