// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/lex/ascii.hpp>

#include "tokenize.hpp"
#include <catch.hpp>
#include <cctype>

TEST_CASE("ascii predicates")
{
    using namespace foonathan::lex::ascii;

    auto verify = [&](char c) {
        REQUIRE(is_ascii(c));

        auto category_count = is_control(c) + is_blank(c) + is_newline(c) + is_other_space(c)
                              + is_digit(c) + is_lower(c) + is_upper(c) + is_punct(c);
        REQUIRE(category_count == 1);

        if (is_blank(c) || is_newline(c) || is_other_space(c))
            REQUIRE(is_space(c));
        else
            REQUIRE(!is_space(c));

        if (is_lower(c) || is_upper(c))
            REQUIRE(is_alpha(c));
        else
            REQUIRE(!is_alpha(c));

        if (is_alpha(c) || is_digit(c))
            REQUIRE(is_alnum(c));
        else
            REQUIRE(!is_alnum(c));

        if (is_alnum(c) || is_punct(c))
            REQUIRE(is_graph(c));
        else
            REQUIRE(!is_graph(c));

        if (is_graph(c) || c == ' ')
            REQUIRE(is_print(c));
        else
            REQUIRE(!is_print(c));

        REQUIRE(is_blank(c) == !!std::isblank(c));
        REQUIRE(is_digit(c) == !!std::isdigit(c));
        REQUIRE(is_lower(c) == !!std::islower(c));
        REQUIRE(is_upper(c) == !!std::isupper(c));
        REQUIRE(is_punct(c) == !!std::ispunct(c));
        REQUIRE(is_space(c) == !!std::isspace(c));
        REQUIRE(is_alpha(c) == !!std::isalpha(c));
        REQUIRE(is_alnum(c) == !!std::isalnum(c));
        REQUIRE(is_graph(c) == !!std::isgraph(c));
        REQUIRE(is_print(c) == !!std::isprint(c));
    };

    for (auto c = 0; c <= 127; ++c)
        verify(static_cast<char>(c));
}

namespace
{
using test_spec = lex::token_spec<struct whitespace, struct digit, struct alpha>;

struct whitespace : lex::rule_token<whitespace, test_spec>
{
    static constexpr auto rule() noexcept
    {
        return lex::token_rule::star(lex::ascii::is_space);
    }
};

struct digit : lex::rule_token<digit, test_spec>
{
    static constexpr auto rule() noexcept
    {
        return lex::ascii::is_digit;
    }
};

struct alpha : lex::rule_token<alpha, test_spec>
{
    static constexpr auto rule() noexcept
    {
        return lex::ascii::is_upper + lex::token_rule::star(lex::ascii::is_lower);
    }
};
} // namespace

TEST_CASE("single_ascii_token and ascii_token")
{
    static constexpr const char       array[]   = "Abcde  12aBB";
    constexpr auto                    tokenizer = lex::tokenizer<test_spec>(array);
    FOONATHAN_LEX_TEST_CONSTEXPR auto result    = tokenize<test_spec>(tokenizer);

    REQUIRE(result.size() == 7);

    REQUIRE(result[0].is(alpha{}));
    REQUIRE(result[0].spelling() == "Abcde");
    REQUIRE(result[0].offset(tokenizer) == 0);

    REQUIRE(result[1].is(whitespace{}));
    REQUIRE(result[1].spelling() == "  ");
    REQUIRE(result[1].offset(tokenizer) == 5);

    REQUIRE(result[2].is(digit{}));
    REQUIRE(result[2].spelling() == "1");
    REQUIRE(result[2].offset(tokenizer) == 7);

    REQUIRE(result[3].is(digit{}));
    REQUIRE(result[3].spelling() == "2");
    REQUIRE(result[3].offset(tokenizer) == 8);

    REQUIRE(result[4].is(lex::error_token{}));
    REQUIRE(result[4].spelling() == "a");
    REQUIRE(result[4].offset(tokenizer) == 9);

    REQUIRE(result[5].is(alpha{}));
    REQUIRE(result[5].spelling() == "B");
    REQUIRE(result[5].offset(tokenizer) == 10);

    REQUIRE(result[6].is(alpha{}));
    REQUIRE(result[6].spelling() == "B");
    REQUIRE(result[6].offset(tokenizer) == 11);
}
