// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/lex/whitespace_token.hpp>

#include "tokenize.hpp"
#include <catch.hpp>

namespace
{
using test_spec = lex::token_spec<struct token_a, struct token_b>;

struct token_a : lex::literal_token<'a'>
{};

struct token_b : lex::literal_token<'b'>, lex::whitespace_token
{};
} // namespace

TEST_CASE("whitespace_token")
{
    static constexpr const char       array[]   = "bbabaabbb";
    constexpr auto                    tokenizer = lex::tokenizer<test_spec>(array);
    FOONATHAN_LEX_TEST_CONSTEXPR auto result    = tokenize<test_spec>(tokenizer);

    REQUIRE(result.size() == 3);

    REQUIRE(result[0].is(token_a{}));
    REQUIRE(result[0].spelling() == "a");
    REQUIRE(result[0].offset(tokenizer) == 2);

    REQUIRE(result[1].is(token_a{}));
    REQUIRE(result[1].spelling() == "a");
    REQUIRE(result[1].offset(tokenizer) == 4);

    REQUIRE(result[2].is(token_a{}));
    REQUIRE(result[2].spelling() == "a");
    REQUIRE(result[2].offset(tokenizer) == 5);
}

TEST_CASE("whitespace_token and reset()")
{
    static constexpr const char array[] = "bbabbba";

    auto tokenizer = lex::tokenizer<test_spec>(array);
    REQUIRE(tokenizer.peek().is(token_a{}));
    REQUIRE(tokenizer.peek().offset(tokenizer) == 2);

    tokenizer.reset(tokenizer.current_ptr() + 1);
    REQUIRE(tokenizer.peek().is(token_a{}));
    REQUIRE(tokenizer.peek().offset(tokenizer) == 6);
}
