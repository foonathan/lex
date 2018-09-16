// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/lex/identifier.hpp>

#include "tokenize.hpp"
#include <catch.hpp>
#include <foonathan/lex/rules.hpp>

namespace
{
using test_spec = lex::token_spec<struct whitespace, struct identifier, struct keyword_a,
                                  struct keyword_ab, struct keyword_c>;

struct whitespace : lex::rule_token<whitespace, test_spec>,
                    lex::loop_ascii_mixin<whitespace, lex::ascii::is_blank>
{};

struct identifier : lex::identifier<identifier, test_spec>
{
    static constexpr match_result try_match(const char* str, const char* end) noexcept
    {
        using lex::ascii::is_alpha;

        if (!is_alpha(*str))
            return unmatched();

        auto start = str;
        while (str != end && is_alpha(*str))
            ++str;

        return ok(str - start);
    }
};

struct keyword_a : lex::keyword<'a'>
{};

struct keyword_ab : lex::keyword<'a', 'b'>
{};

struct keyword_c : lex::keyword<'c'>
{};
} // namespace

TEST_CASE("identifier and keyword")
{
    static constexpr const char array[] = "dd a ab abc c";
    constexpr auto              result  = tokenize<test_spec>(array);

    REQUIRE(result.size() == 9);

    REQUIRE(result[0].is(identifier{}));
    REQUIRE(result[0].spelling() == "dd");
    REQUIRE(result[0].offset() == 0);

    REQUIRE(result[1].is(whitespace{}));
    REQUIRE(result[1].spelling() == " ");
    REQUIRE(result[1].offset() == 2);

    REQUIRE(result[2].is(keyword_a{}));
    REQUIRE(result[2].spelling() == "a");
    REQUIRE(result[2].offset() == 3);

    REQUIRE(result[3].is(whitespace{}));
    REQUIRE(result[3].spelling() == " ");
    REQUIRE(result[3].offset() == 4);

    REQUIRE(result[4].is(keyword_ab{}));
    REQUIRE(result[4].spelling() == "ab");
    REQUIRE(result[4].offset() == 5);

    REQUIRE(result[5].is(whitespace{}));
    REQUIRE(result[5].spelling() == " ");
    REQUIRE(result[5].offset() == 7);

    REQUIRE(result[6].is(identifier{}));
    REQUIRE(result[6].spelling() == "abc");
    REQUIRE(result[6].offset() == 8);

    REQUIRE(result[7].is(whitespace{}));
    REQUIRE(result[7].spelling() == " ");
    REQUIRE(result[7].offset() == 11);

    REQUIRE(result[8].is(keyword_c{}));
    REQUIRE(result[8].spelling() == "c");
    REQUIRE(result[8].offset() == 12);
}
