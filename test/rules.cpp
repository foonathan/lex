// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/lex/rules.hpp>

#include "tokenize.hpp"
#include <catch.hpp>
#include <locale>

namespace lex = foonathan::lex;

TEST_CASE("ascii predicates")
{
    using namespace lex::ascii;

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

        auto& c_locale = std::locale::classic();
        REQUIRE(is_blank(c) == std::isblank(c, c_locale));
        REQUIRE(is_digit(c) == std::isdigit(c, c_locale));
        REQUIRE(is_lower(c) == std::islower(c, c_locale));
        REQUIRE(is_upper(c) == std::isupper(c, c_locale));
        REQUIRE(is_punct(c) == std::ispunct(c, c_locale));
        REQUIRE(is_space(c) == std::isspace(c, c_locale));
        REQUIRE(is_alpha(c) == std::isalpha(c, c_locale));
        REQUIRE(is_alnum(c) == std::isalnum(c, c_locale));
        REQUIRE(is_graph(c) == std::isgraph(c, c_locale));
        REQUIRE(is_print(c) == std::isprint(c, c_locale));
    };

    for (auto c = 0; c <= 127; ++c)
        verify(static_cast<char>(c));
}

namespace
{
using test_spec = lex::token_spec<struct whitespace, struct digit, struct alpha>;

struct whitespace : lex::rule_token<whitespace, test_spec>,
                    lex::loop_ascii_mixin<whitespace, lex::ascii::is_space>
{};

struct digit : lex::rule_token<digit, test_spec>,
               lex::single_ascii_mixin<digit, lex::ascii::is_digit>
{};

struct alpha : lex::rule_token<alpha, test_spec>,
               lex::head_tail_ascii_mixin<alpha, lex::ascii::is_upper, lex::ascii::is_lower>
{};
} // namespace

TEST_CASE("single_ascii_token and ascii_token")
{
    static constexpr const char array[]   = "Abcde  12aBB";
    constexpr auto              tokenizer = lex::tokenizer<test_spec>(array);
    constexpr auto              result    = tokenize<test_spec>(tokenizer);

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

    REQUIRE(result[4].is_error());
    REQUIRE(result[4].spelling() == "a");
    REQUIRE(result[4].offset(tokenizer) == 9);

    REQUIRE(result[5].is(alpha{}));
    REQUIRE(result[5].spelling() == "B");
    REQUIRE(result[5].offset(tokenizer) == 10);

    REQUIRE(result[6].is(alpha{}));
    REQUIRE(result[6].spelling() == "B");
    REQUIRE(result[6].offset(tokenizer) == 11);
}
