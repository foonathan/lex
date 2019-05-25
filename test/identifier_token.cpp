// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/lex/identifier_token.hpp>

#include <doctest.h>
#include <string>

#include <foonathan/lex/ascii.hpp>
#include "tokenize.hpp"

namespace
{
using test_spec = lex::token_spec<struct whitespace, struct identifier, struct keyword_a,
                                  struct keyword_ab, struct keyword_c>;

struct whitespace : lex::rule_token<whitespace, test_spec>
{
    static constexpr const char* name = "<whitespace>";

    static constexpr auto rule() noexcept
    {
        return lex::token_rule::star(lex::ascii::is_blank);
    }
};

struct identifier : lex::identifier_token<identifier, test_spec>
{
    static constexpr match_result try_match(const char* str, const char* end) noexcept
    {
        using lex::ascii::is_alpha;

        if (!is_alpha(*str))
            return unmatched();

        auto start = str;
        while (str != end && is_alpha(*str))
            ++str;

        return success(str - start);
    }
};

struct keyword_a : lex::keyword_token<'a'>
{};

struct keyword_ab : FOONATHAN_LEX_KEYWORD("ab")
{};

struct keyword_c : lex::keyword_token<'c'>
{};
} // namespace

TEST_CASE("identifier_token and keyword_token")
{
    static constexpr const char       array[]   = "dd a ab abc c d";
    constexpr auto                    tokenizer = lex::tokenizer<test_spec>(array);
    FOONATHAN_LEX_TEST_CONSTEXPR auto result    = tokenize<test_spec>(tokenizer);

    REQUIRE(result.size() == 11);

    REQUIRE(result[0].is(identifier{}));
    REQUIRE(result[0].name() == std::string("<identifier>"));
    REQUIRE(result[0].spelling() == "dd");
    REQUIRE(result[0].offset(tokenizer) == 0);

    REQUIRE(result[1].is(whitespace{}));
    REQUIRE(result[1].name() == std::string("<whitespace>"));
    REQUIRE(result[1].spelling() == " ");
    REQUIRE(result[1].offset(tokenizer) == 2);

    REQUIRE(result[2].is(keyword_a{}));
    REQUIRE(result[2].name() == std::string("a"));
    REQUIRE(result[2].spelling() == "a");
    REQUIRE(result[2].offset(tokenizer) == 3);

    REQUIRE(result[3].is(whitespace{}));
    REQUIRE(result[3].spelling() == " ");
    REQUIRE(result[3].offset(tokenizer) == 4);

    REQUIRE(result[4].is(keyword_ab{}));
    REQUIRE(result[4].name() == std::string("ab"));
    REQUIRE(result[4].spelling() == "ab");
    REQUIRE(result[4].offset(tokenizer) == 5);

    REQUIRE(result[5].is(whitespace{}));
    REQUIRE(result[5].spelling() == " ");
    REQUIRE(result[5].offset(tokenizer) == 7);

    REQUIRE(result[6].is(identifier{}));
    REQUIRE(result[6].spelling() == "abc");
    REQUIRE(result[6].offset(tokenizer) == 8);

    REQUIRE(result[7].is(whitespace{}));
    REQUIRE(result[7].spelling() == " ");
    REQUIRE(result[7].offset(tokenizer) == 11);

    REQUIRE(result[8].is(keyword_c{}));
    REQUIRE(result[8].name() == std::string("c"));
    REQUIRE(result[8].spelling() == "c");
    REQUIRE(result[8].offset(tokenizer) == 12);

    REQUIRE(result[9].is(whitespace{}));
    REQUIRE(result[9].spelling() == " ");
    REQUIRE(result[9].offset(tokenizer) == 13);

    REQUIRE(result[10].is(identifier{}));
    REQUIRE(result[10].spelling() == "d");
    REQUIRE(result[10].offset(tokenizer) == 14);
}
