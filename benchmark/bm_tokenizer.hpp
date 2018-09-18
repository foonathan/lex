// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_BM_TOKENIZER_HPP_INCLUDED
#define FOONATHAN_LEX_BM_TOKENIZER_HPP_INCLUDED

#include <foonathan/lex/ascii.hpp>
#include <foonathan/lex/tokenizer.hpp>

namespace tokenizer_ns
{
namespace lex = foonathan::lex;

using token_spec
    = lex::token_spec<struct ellipsis, struct dot, struct plus_eq, struct plus_plus, struct plus,
                      struct arrow_deref, struct arrow, struct minus_minus, struct minus_eq,
                      struct minus, struct tilde, struct whitespace>;

struct ellipsis : lex::literal_token<'.', '.', '.'>
{};

struct dot : lex::literal_token<'.'>
{};

struct plus_eq : lex::literal_token<'+', '='>
{};

struct plus_plus : lex::literal_token<'+', '+'>
{};

struct plus : lex::literal_token<'+'>
{};

struct arrow_deref : lex::literal_token<'-', '>', '*'>
{};

struct arrow : lex::literal_token<'-', '>'>
{};

struct minus_minus : lex::literal_token<'-', '-'>
{};

struct minus : lex::literal_token<'-'>
{};

struct tilde : lex::literal_token<'~'>
{};

struct whitespace : lex::rule_token<whitespace, token_spec>,
                    lex::loop_ascii_mixin<whitespace, lex::ascii::is_space>
{};
} // namespace tokenizer_ns

void tokenizer(const char* str, const char* end, void (*f)(foonathan::lex::token_spelling))
{
    using namespace tokenizer_ns;
    namespace lex = foonathan::lex;

    lex::tokenizer<token_spec> tokenizer(str, end);
    while (!tokenizer.is_eof())
    {
        auto cur = tokenizer.peek();
        if (!cur.is_error())
            f(cur.spelling());
        tokenizer.bump();
    }
}

#endif // FOONATHAN_LEX_BM_TOKENIZER_HPP_INCLUDED
