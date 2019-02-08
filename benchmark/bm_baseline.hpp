// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_BM_BASELINE_HPP_INCLUDED
#define FOONATHAN_LEX_BM_BASELINE_HPP_INCLUDED

#include <foonathan/lex/spelling.hpp>

void baseline(const char* str, const char* end, void (*f)(int, foonathan::lex::token_spelling))
{
    namespace lex = foonathan::lex;

    while (str != end)
    {
        f(0, lex::token_spelling(str, 1));
        ++str;
    }
}

#endif // FOONATHAN_LEX_BM_BASELINE_HPP_INCLUDED
