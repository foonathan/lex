// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/lex/literal_token.hpp>

#include <doctest.h>
#include <string>

#include "tokenize.hpp"

namespace
{
using test_spec = lex::token_spec<struct token_a, struct token_abc, struct token_bc>;

struct token_a : lex::literal_token<'a'>
{};

struct token_abc : FOONATHAN_LEX_LITERAL("abc")
{};

struct token_bc : FOONATHAN_LEX_LITERAL("bc")
{};
} // namespace

TEST_CASE("literal_token")
{
    SUBCASE("token_a")
    {
        static constexpr const char       array[]   = "aaa";
        constexpr auto                    tokenizer = lex::tokenizer<test_spec>(array);
        FOONATHAN_LEX_TEST_CONSTEXPR auto result    = tokenize<test_spec>(tokenizer);

        REQUIRE(result.size() == 3);

        REQUIRE(result[0].is(token_a{}));
        REQUIRE(result[0].name() == std::string("a"));
        REQUIRE(result[0].spelling() == "a");
        REQUIRE(result[0].offset(tokenizer) == 0);

        REQUIRE(result[1].is(token_a{}));
        REQUIRE(result[1].spelling() == "a");
        REQUIRE(result[1].offset(tokenizer) == 1);

        REQUIRE(result[2].is(token_a{}));
        REQUIRE(result[2].spelling() == "a");
        REQUIRE(result[2].offset(tokenizer) == 2);
    }
    SUBCASE("mixed")
    {
        static constexpr const char       array[]   = "abcaabbc";
        constexpr auto                    tokenizer = lex::tokenizer<test_spec>(array);
        FOONATHAN_LEX_TEST_CONSTEXPR auto result    = tokenize<test_spec>(tokenizer);

        REQUIRE(result.size() == 5);

        REQUIRE(result[0].is(token_abc{}));
        REQUIRE(result[0].name() == std::string("abc"));
        REQUIRE(result[0].spelling() == "abc");
        REQUIRE(result[0].offset(tokenizer) == 0);

        REQUIRE(result[1].is(token_a{}));
        REQUIRE(result[1].spelling() == "a");
        REQUIRE(result[1].offset(tokenizer) == 3);

        REQUIRE(result[2].is(token_a{}));
        REQUIRE(result[2].spelling() == "a");
        REQUIRE(result[2].offset(tokenizer) == 4);

        REQUIRE(result[3].is(lex::error_token{}));
        REQUIRE(result[3].spelling() == "b");
        REQUIRE(result[3].offset(tokenizer) == 5);

        REQUIRE(result[4].is(token_bc{}));
        REQUIRE(result[4].name() == std::string("bc"));
        REQUIRE(result[4].spelling() == "bc");
        REQUIRE(result[4].offset(tokenizer) == 6);
    }
}
