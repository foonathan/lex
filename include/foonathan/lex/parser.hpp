// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_PARSER_HPP_INCLUDED
#define FOONATHAN_LEX_PARSER_HPP_INCLUDED

#include <foonathan/lex/grammar.hpp>
#include <foonathan/lex/parse_result.hpp>
#include <foonathan/lex/tokenizer.hpp>

namespace foonathan
{
namespace lex
{
    template <class Grammar, class Func>
    constexpr auto parse(tokenizer<typename Grammar::token_spec>& tokenizer, Func&& f)
    {
        return Grammar::start::parse(tokenizer, f);
    }
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_PARSER_HPP_INCLUDED
