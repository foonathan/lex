// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/lex/tokenizer.hpp>

#include <catch.hpp>

namespace lex = foonathan::lex;

namespace
{
using test_spec = lex::token_spec<struct token_a, struct token_bc>;

struct token_a : FOONATHAN_LEX_LITERAL("a")
{};

struct token_bc : FOONATHAN_LEX_LITERAL("bc")
{};

template <class Token>
void verify(const lex::tokenizer<test_spec>& tokenizer, const char* ptr, bool is_done)
{
    REQUIRE(tokenizer.current_ptr() == ptr);
    REQUIRE(tokenizer.is_done() == is_done);

    REQUIRE(tokenizer.peek().is(Token{}));
    REQUIRE(tokenizer.peek().spelling().data() == tokenizer.current_ptr());
}
} // namespace

TEST_CASE("tokenizer")
{
    const char                array[] = "abc aabc";
    lex::tokenizer<test_spec> tokenizer(array);
    REQUIRE(tokenizer.begin_ptr() == array);
    REQUIRE(tokenizer.end_ptr() == array + sizeof(array) - 1);

    verify<token_a>(tokenizer, array, false);

    tokenizer.bump();
    verify<token_bc>(tokenizer, array + 1, false);

    tokenizer.bump();
    verify<lex::error_token>(tokenizer, array + 3, false);

    auto token = tokenizer.get();
    REQUIRE(token.is(lex::error_token{}));
    REQUIRE(token.spelling().data() == array + 3);
    verify<token_a>(tokenizer, array + 4, false);

    tokenizer.reset(array);
    verify<token_a>(tokenizer, array, false);

    tokenizer.reset(array + 6);
    verify<token_bc>(tokenizer, array + 6, false);

    tokenizer.bump();
    verify<lex::eof_token>(tokenizer, array + 8, true);

    tokenizer.bump();
    verify<lex::eof_token>(tokenizer, array + 8, true);
}
