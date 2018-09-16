// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_TOKENIZE_HPP_INCLUDED
#define FOONATHAN_LEX_TOKENIZE_HPP_INCLUDED

#include <foonathan/lex/tokenizer.hpp>

namespace
{
namespace lex = foonathan::lex;

template <class Spec>
using vector = lex::detail::constexpr_vector<lex::token<Spec>, 16>;

template <class Spec>
constexpr vector<Spec> tokenize(lex::tokenizer<Spec> tokenizer)
{
    vector<Spec> result;

    while (!tokenizer.is_eof())
        result.push_back(tokenizer.get());

    return result;
}
} // namespace

#endif // FOONATHAN_LEX_TOKENIZE_HPP_INCLUDED
