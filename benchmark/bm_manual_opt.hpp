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
        // don't call function directly,
        // go to memory store first
        // this makes the benchmark more fair:
        // a real tokenizer won't be able to handle each token directly as well
        auto id   = -1;
        auto size = 0u;
        switch (*str)
        {
        case '.':
            if (str + 1 != end && str[1] == '.' && str + 2 != end && str[2] == '.')
                id = 0, size = 3;
            else
                id = 1, size = 1;
            break;

        case '+':
            if (str + 1 != end)
                switch (str[1])
                {
                case '=':
                    id = 2, size = 2;
                    break;
                case '+':
                    id = 3, size = 2;
                    break;
                default:
                    id = 4, size = 1;
                    break;
                }
            else
                id = 4, size = 1;
            break;

        case '-':
            if (str + 1 != end)
                switch (str[1])
                {
                case '>':
                    if (str + 2 != end && str[2] == '*')
                        id = 5, size = 3;
                    else
                        id = 6, size = 2;
                case '-':
                    id = 7, size = 2;
                    break;
                case '=':
                    id = 8, size = 2;
                    break;

                default:
                    id = 9, size = 1;
                }
            else
                id = 9, size = 1;
            break;

        case '~':
            id = 10, size = 1;
            break;

        case ' ':
        case '\t':
        case '\n':
        case '\r':
        case '\f':
        case '\v': // whitespace
        {
            auto cur = str;
            while (cur != end && lex::ascii::is_space(*cur))
                ++cur;
            id   = 11;
            size = static_cast<unsigned>(cur - str);
            break;
        }

        default:
            ++str;
            break;
        }

        if (size > 0)
            f(id, bump(str, size));
    }
}

#endif // FOONATHAN_LEX_BM_MANUAL_OPT_HPP_INCLUDED
