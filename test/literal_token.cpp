// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/lex/literal_token.hpp>

#include <catch.hpp>
#include "tokenize.hpp"

namespace
{
    using test_spec = lex::token_spec<struct token_a, struct token_abc, struct token_bc>;

    struct token_a : lex::literal_token<'a'>
    {
    };

    struct token_abc : lex::literal_token<'a', 'b', 'c'>
    {
    };

    struct token_bc : lex::literal_token<'b', 'c'>
    {
    };
} // namespace

TEST_CASE("literal_token")
{
    SECTION("token_a")
    {
        static constexpr const char array[] = "aaa";
        constexpr auto              result  = tokenize<test_spec>(array);

        REQUIRE(result.size() == 3);

        REQUIRE(result[0].is(token_a{}));
        REQUIRE(result[0].spelling() == "a");
        REQUIRE(result[0].offset() == 0);

        REQUIRE(result[1].is(token_a{}));
        REQUIRE(result[1].spelling() == "a");
        REQUIRE(result[1].offset() == 1);

        REQUIRE(result[2].is(token_a{}));
        REQUIRE(result[2].spelling() == "a");
        REQUIRE(result[2].offset() == 2);
    }
    SECTION("mixed")
    {
        static constexpr const char array[] = "abcaabbc";
        constexpr auto              result  = tokenize<test_spec>(array);

        REQUIRE(result.size() == 5);

        REQUIRE(result[0].is(token_abc{}));
        REQUIRE(result[0].spelling() == "abc");
        REQUIRE(result[0].offset() == 0);

        REQUIRE(result[1].is(token_a{}));
        REQUIRE(result[1].spelling() == "a");
        REQUIRE(result[1].offset() == 3);

        REQUIRE(result[2].is(token_a{}));
        REQUIRE(result[2].spelling() == "a");
        REQUIRE(result[2].offset() == 4);

        REQUIRE(result[3].is_error());
        REQUIRE(result[3].spelling() == "b");
        REQUIRE(result[3].offset() == 5);

        REQUIRE(result[4].is(token_bc{}));
        REQUIRE(result[4].spelling() == "bc");
        REQUIRE(result[4].offset() == 6);
    }
}
