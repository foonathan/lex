// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/lex/tokenizer.hpp>

#include <catch.hpp>
#include <foonathan/lex/detail/constexpr_vector.hpp>

namespace lex = foonathan::lex;

namespace
{
    using test_spec = lex::token_spec<struct token_a, struct token_bc>;

    // token_a: an even number of 'a'
    struct token_a : lex::rule_token<test_spec>
    {
        static constexpr lex::parse_rule_result<test_spec> try_match(const char* str,
                                                                     const char* end) noexcept
        {
            if (*str != 'a')
                return lex::parse_rule_result<test_spec>();

            auto start = str;
            while (str != end && *str == 'a')
                ++str;
            auto count = std::size_t(str - start);

            if (count % 2 == 0)
                return lex::parse_rule_result<test_spec>(lex::token_kind<test_spec>(token_a{}),
                                                         count);
            else
                return lex::parse_rule_result<test_spec>(count);
        }
    };

    // token_bc: 'bc'
    struct token_bc : lex::rule_token<test_spec>
    {
        static constexpr lex::parse_rule_result<test_spec> try_match(const char* str,
                                                                     const char* end) noexcept
        {
            if (*str == 'b' & str + 1 != end && str[1] == 'c')
                return lex::parse_rule_result<test_spec>(lex::token_kind<test_spec>(token_bc{}), 2);
            else
                return lex::parse_rule_result<test_spec>();
        }
    };

    using vector = lex::detail::constexpr_vector<lex::token<test_spec>, 16>;

    template <std::size_t N>
    constexpr vector tokenize(const char (&array)[N])
    {
        vector result;

        lex::tokenizer<test_spec> tokenizer(array, N);
        while (!tokenizer.is_eof())
            result.push_back(tokenizer.get());

        return result;
    }
} // namespace

TEST_CASE("rule_token")
{
    SECTION("token_a")
    {
        static constexpr const char array[] = "aaaa";
        constexpr auto              result  = tokenize(array);

        REQUIRE(result.size() == 1);

        REQUIRE(result[0].is(token_a{}));
        REQUIRE(result[0].spelling() == "aaaa");
        REQUIRE(result[0].offset() == 0);
    }
    SECTION("token_a error")
    {
        static constexpr const char array[] = "aaa";
        constexpr auto              result  = tokenize(array);

        REQUIRE(result.size() == 1);

        REQUIRE(result[0].is_error());
        REQUIRE(result[0].spelling() == "aaa");
        REQUIRE(result[0].offset() == 0);
    }
    SECTION("token_bc")
    {
        static constexpr const char array[] = "bcbc";
        constexpr auto              result  = tokenize(array);

        REQUIRE(result.size() == 2);

        REQUIRE(result[0].is(token_bc{}));
        REQUIRE(result[0].spelling() == "bc");
        REQUIRE(result[0].offset() == 0);

        REQUIRE(result[1].is(token_bc{}));
        REQUIRE(result[1].spelling() == "bc");
        REQUIRE(result[1].offset() == 2);
    }
    SECTION("token_bc error")
    {
        static constexpr const char array[] = "bbc";
        constexpr auto              result  = tokenize(array);

        REQUIRE(result.size() == 2);

        REQUIRE(result[0].is_error());
        REQUIRE(result[0].spelling() == "b");
        REQUIRE(result[0].offset() == 0);

        REQUIRE(result[1].is(token_bc{}));
        REQUIRE(result[1].spelling() == "bc");
        REQUIRE(result[1].offset() == 1);
    }
    SECTION("mixed")
    {
        static constexpr const char array[] = "aabcabaa";
        constexpr auto              result  = tokenize(array);

        REQUIRE(result.size() == 5);

        REQUIRE(result[0].is(token_a{}));
        REQUIRE(result[0].spelling() == "aa");
        REQUIRE(result[0].offset() == 0);

        REQUIRE(result[1].is(token_bc{}));
        REQUIRE(result[1].spelling() == "bc");
        REQUIRE(result[1].offset() == 2);

        REQUIRE(result[2].is_error());
        REQUIRE(result[2].spelling() == "a");
        REQUIRE(result[2].offset() == 4);

        REQUIRE(result[3].is_error());
        REQUIRE(result[3].spelling() == "b");
        REQUIRE(result[3].offset() == 5);

        REQUIRE(result[4].is(token_a{}));
        REQUIRE(result[4].spelling() == "aa");
        REQUIRE(result[4].offset() == 6);
    }
}
