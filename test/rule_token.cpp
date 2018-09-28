// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/lex/rule_token.hpp>

#include "tokenize.hpp"
#include <catch.hpp>

namespace
{
using test_spec = lex::token_spec<struct token_a, struct token_c, struct token_bc>;

// token_a: an even number of 'a'
struct token_a : lex::rule_token<token_a, test_spec>
{
    static constexpr match_result try_match(const char* str, const char* end) noexcept
    {
        if (*str != 'a')
            return unmatched();

        auto start = str;
        while (str != end && *str == 'a')
            ++str;
        auto count = std::size_t(str - start);

        if (count % 2 == 0)
            return success(count);
        else
            return error(count);
    }
};

// token_c: 'c'
struct token_c : lex::null_token
{};

// token_bc: 'bc'
struct token_bc : lex::rule_token<token_bc, test_spec>
{
    static constexpr match_result try_match(const char* str, const char* end) noexcept
    {
        if (*str == 'b' & str + 1 != end && str[1] == 'c')
            return success(2);
        else if (*str == 'c')
            return success<token_c>(1);
        else
            return unmatched();
    }
};
} // namespace

TEST_CASE("rule_token")
{
    SECTION("token_a")
    {
        static constexpr const char array[]   = "aaaa";
        constexpr auto              tokenizer = lex::tokenizer<test_spec>(array);
        constexpr auto              result    = tokenize<test_spec>(tokenizer);

        REQUIRE(result.size() == 1);

        REQUIRE(result[0].is(token_a{}));
        REQUIRE(result[0].spelling() == "aaaa");
        REQUIRE(result[0].offset(tokenizer) == 0);
    }
    SECTION("token_a error")
    {
        static constexpr const char array[]   = "aaa";
        constexpr auto              tokenizer = lex::tokenizer<test_spec>(array);
        constexpr auto              result    = tokenize<test_spec>(tokenizer);

        REQUIRE(result.size() == 1);

        REQUIRE(result[0].is(lex::error{}));
        REQUIRE(result[0].spelling() == "aaa");
        REQUIRE(result[0].offset(tokenizer) == 0);
    }
    SECTION("token_bc")
    {
        static constexpr const char array[]   = "bccbc";
        constexpr auto              tokenizer = lex::tokenizer<test_spec>(array);
        constexpr auto              result    = tokenize<test_spec>(tokenizer);

        REQUIRE(result.size() == 3);

        REQUIRE(result[0].is(token_bc{}));
        REQUIRE(result[0].spelling() == "bc");
        REQUIRE(result[0].offset(tokenizer) == 0);

        REQUIRE(result[1].is(token_c{}));
        REQUIRE(result[1].spelling() == "c");
        REQUIRE(result[1].offset(tokenizer) == 2);

        REQUIRE(result[2].is(token_bc{}));
        REQUIRE(result[2].spelling() == "bc");
        REQUIRE(result[2].offset(tokenizer) == 3);
    }
    SECTION("token_bc error")
    {
        static constexpr const char array[]   = "bbc";
        constexpr auto              tokenizer = lex::tokenizer<test_spec>(array);
        constexpr auto              result    = tokenize<test_spec>(tokenizer);

        REQUIRE(result.size() == 2);

        REQUIRE(result[0].is(lex::error{}));
        REQUIRE(result[0].spelling() == "b");
        REQUIRE(result[0].offset(tokenizer) == 0);

        REQUIRE(result[1].is(token_bc{}));
        REQUIRE(result[1].spelling() == "bc");
        REQUIRE(result[1].offset(tokenizer) == 1);
    }
    SECTION("mixed")
    {
        static constexpr const char array[]   = "aabcabaa";
        constexpr auto              tokenizer = lex::tokenizer<test_spec>(array);
        constexpr auto              result    = tokenize<test_spec>(tokenizer);

        REQUIRE(result.size() == 5);

        REQUIRE(result[0].is(token_a{}));
        REQUIRE(result[0].spelling() == "aa");
        REQUIRE(result[0].offset(tokenizer) == 0);

        REQUIRE(result[1].is(token_bc{}));
        REQUIRE(result[1].spelling() == "bc");
        REQUIRE(result[1].offset(tokenizer) == 2);

        REQUIRE(result[2].is(lex::error{}));
        REQUIRE(result[2].spelling() == "a");
        REQUIRE(result[2].offset(tokenizer) == 4);

        REQUIRE(result[3].is(lex::error{}));
        REQUIRE(result[3].spelling() == "b");
        REQUIRE(result[3].offset(tokenizer) == 5);

        REQUIRE(result[4].is(token_a{}));
        REQUIRE(result[4].spelling() == "aa");
        REQUIRE(result[4].offset(tokenizer) == 6);
    }
}
