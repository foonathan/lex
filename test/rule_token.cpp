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
struct token_a : lex::basic_rule_token<token_a, test_spec>
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
struct token_bc : lex::basic_rule_token<token_bc, test_spec>
{
    static constexpr match_result try_match(const char* str, const char* end) noexcept
    {
        if (*str == 'b' && str + 1 != end && str[1] == 'c')
            return success(2);
        else if (*str == 'c')
            return success<token_c>(1);
        else
            return unmatched();
    }
};
} // namespace

TEST_CASE("basic_rule_token")
{
    SECTION("token_a")
    {
        static constexpr const char       array[]   = "aaaa";
        constexpr auto                    tokenizer = lex::tokenizer<test_spec>(array);
        FOONATHAN_LEX_TEST_CONSTEXPR auto result    = tokenize<test_spec>(tokenizer);

        REQUIRE(result.size() == 1);

        REQUIRE(result[0].is(token_a{}));
        REQUIRE(result[0].spelling() == "aaaa");
        REQUIRE(result[0].offset(tokenizer) == 0);
    }
    SECTION("token_a error")
    {
        static constexpr const char       array[]   = "aaa";
        constexpr auto                    tokenizer = lex::tokenizer<test_spec>(array);
        FOONATHAN_LEX_TEST_CONSTEXPR auto result    = tokenize<test_spec>(tokenizer);

        REQUIRE(result.size() == 1);

        REQUIRE(result[0].is(lex::error_token{}));
        REQUIRE(result[0].spelling() == "aaa");
        REQUIRE(result[0].offset(tokenizer) == 0);
    }
    SECTION("token_bc")
    {
        static constexpr const char       array[]   = "bccbc";
        constexpr auto                    tokenizer = lex::tokenizer<test_spec>(array);
        FOONATHAN_LEX_TEST_CONSTEXPR auto result    = tokenize<test_spec>(tokenizer);

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
        static constexpr const char       array[]   = "bbc";
        constexpr auto                    tokenizer = lex::tokenizer<test_spec>(array);
        FOONATHAN_LEX_TEST_CONSTEXPR auto result    = tokenize<test_spec>(tokenizer);

        REQUIRE(result.size() == 2);

        REQUIRE(result[0].is(lex::error_token{}));
        REQUIRE(result[0].spelling() == "b");
        REQUIRE(result[0].offset(tokenizer) == 0);

        REQUIRE(result[1].is(token_bc{}));
        REQUIRE(result[1].spelling() == "bc");
        REQUIRE(result[1].offset(tokenizer) == 1);
    }
    SECTION("mixed")
    {
        static constexpr const char       array[]   = "aabcabaa";
        constexpr auto                    tokenizer = lex::tokenizer<test_spec>(array);
        FOONATHAN_LEX_TEST_CONSTEXPR auto result    = tokenize<test_spec>(tokenizer);

        REQUIRE(result.size() == 5);

        REQUIRE(result[0].is(token_a{}));
        REQUIRE(result[0].spelling() == "aa");
        REQUIRE(result[0].offset(tokenizer) == 0);

        REQUIRE(result[1].is(token_bc{}));
        REQUIRE(result[1].spelling() == "bc");
        REQUIRE(result[1].offset(tokenizer) == 2);

        REQUIRE(result[2].is(lex::error_token{}));
        REQUIRE(result[2].spelling() == "a");
        REQUIRE(result[2].offset(tokenizer) == 4);

        REQUIRE(result[3].is(lex::error_token{}));
        REQUIRE(result[3].spelling() == "b");
        REQUIRE(result[3].offset(tokenizer) == 5);

        REQUIRE(result[4].is(token_a{}));
        REQUIRE(result[4].spelling() == "aa");
        REQUIRE(result[4].offset(tokenizer) == 6);
    }
}

namespace
{
template <class PEG, std::size_t N>
constexpr auto match(const char (&str)[N])
{
    struct token;
    using spec = lex::token_spec<token>;
    struct token : lex::rule_token<token, spec>
    {
        static constexpr auto rule() noexcept
        {
            return PEG::rule();
        }
    };

    return token::try_match(str, str + N - 1);
}

template <class PEG, std::size_t N>
bool verify(const char (&str)[N], std::size_t length)
{
    auto result = match<PEG>(str);
    if (length == 0)
        return !result.is_matched();
    else
        return result.is_success() && result.bump == length;
}

#define FOONATHAN_LEX_PEG(...)                                                                     \
    struct PEG                                                                                     \
    {                                                                                              \
        static constexpr auto rule() noexcept                                                      \
        {                                                                                          \
            using namespace lex::token_rule;                                                       \
            return __VA_ARGS__;                                                                    \
        }                                                                                          \
    };

} // namespace

TEST_CASE("rule_token")
{
    SECTION("atomic rules")
    {
        SECTION("character")
        {
            FOONATHAN_LEX_PEG('a');

            REQUIRE(verify<PEG>("a", 1));
            REQUIRE(verify<PEG>("aa", 1));
            REQUIRE(verify<PEG>("b", 0));
        }
        SECTION("string")
        {
            FOONATHAN_LEX_PEG("abc");

            REQUIRE(verify<PEG>("abc", 3));
            REQUIRE(verify<PEG>("abcd", 3));
            REQUIRE(verify<PEG>("ab", 0));
            REQUIRE(verify<PEG>("abd", 0));
            REQUIRE(verify<PEG>("bcd", 0));
        }
        SECTION("predicate")
        {
            struct predicate
            {
                constexpr bool operator()(char c) const noexcept
                {
                    return c == 'a' || c == 'b';
                }
            };
            FOONATHAN_LEX_PEG(predicate{});

            REQUIRE(verify<PEG>("ab", 1));
            REQUIRE(verify<PEG>("ba", 1));
            REQUIRE(verify<PEG>("c", 0));
        }
        SECTION("callable")
        {
            struct callable
            {
                constexpr std::size_t operator()(const char* cur, const char* end) const noexcept
                {
                    if (end - cur != 2)
                        return 0;
                    else if (cur[0] == 'a' && cur[1] == 'b')
                        return 1;
                    else
                        return 0;
                }
            };
            FOONATHAN_LEX_PEG(callable{});

            REQUIRE(verify<PEG>("ab", 1));
            REQUIRE(verify<PEG>("abc", 0));
            REQUIRE(verify<PEG>("ba", 0));
        }
        SECTION("any")
        {
            FOONATHAN_LEX_PEG(any);

            REQUIRE(verify<PEG>("abc", 1));
            REQUIRE(verify<PEG>("bca", 1));
            REQUIRE(verify<PEG>("", 0));
        }
        SECTION("skip")
        {
            FOONATHAN_LEX_PEG(skip<2>);

            REQUIRE(verify<PEG>("abc", 2));
            REQUIRE(verify<PEG>("abc", 2));
            REQUIRE(verify<PEG>("a", 0));
        }
        SECTION("eof")
        {
            // have to do a complex test
            FOONATHAN_LEX_PEG(any + eof);

            REQUIRE(verify<PEG>("a", 1));
            REQUIRE(verify<PEG>("abc", 0));
            REQUIRE(verify<PEG>("", 0));
        }
        SECTION("fail")
        {
            FOONATHAN_LEX_PEG(fail);

            REQUIRE(verify<PEG>("abc", 0));
            REQUIRE(verify<PEG>("", 0));
        }
    }
    SECTION("combinators")
    {
        SECTION("sequence")
        {
            FOONATHAN_LEX_PEG(r('a') + 'b' + 'c');

            REQUIRE(verify<PEG>("abc", 3));
            REQUIRE(verify<PEG>("abcd", 3));
            REQUIRE(verify<PEG>("acd", 0));
            REQUIRE(verify<PEG>("bcd", 0));
            REQUIRE(verify<PEG>("", 0));
        }
        SECTION("choice")
        {
            // note: "abc" can never match as it is ordered
            FOONATHAN_LEX_PEG(r("ab") / 'a' / "abc" / 'c');

            REQUIRE(verify<PEG>("ab", 2));
            REQUIRE(verify<PEG>("a", 1));
            REQUIRE(verify<PEG>("c", 1));
            REQUIRE(verify<PEG>("abc", 2));
            REQUIRE(verify<PEG>("bc", 0));
        }
        SECTION("opt")
        {
            FOONATHAN_LEX_PEG('a' + opt('b'));

            REQUIRE(verify<PEG>("a", 1));
            REQUIRE(verify<PEG>("ab", 2));
            REQUIRE(verify<PEG>("abb", 2));
            REQUIRE(verify<PEG>("bb", 0));
        }
        SECTION("star")
        {
            FOONATHAN_LEX_PEG(star('a'));

            REQUIRE(verify<PEG>("aaa", 3));
            REQUIRE(verify<PEG>("aa", 2));
            REQUIRE(verify<PEG>("a", 1));
            REQUIRE(verify<PEG>("aab", 2));
            // note: token only parsed if rule didn't match zero characters
            REQUIRE(verify<PEG>("b", 0));
            REQUIRE(verify<PEG>("", 0));
        }
        SECTION("plus")
        {
            FOONATHAN_LEX_PEG(plus('a'));

            REQUIRE(verify<PEG>("aaa", 3));
            REQUIRE(verify<PEG>("aa", 2));
            REQUIRE(verify<PEG>("a", 1));
            REQUIRE(verify<PEG>("aab", 2));
            REQUIRE(verify<PEG>("b", 0));
            REQUIRE(verify<PEG>("", 0));
        }
        SECTION("lookahead")
        {
            FOONATHAN_LEX_PEG(lookahead("ab") + 'a');

            REQUIRE(verify<PEG>("abc", 1));
            REQUIRE(verify<PEG>("a", 0));
            REQUIRE(verify<PEG>("", 0));
        }
        SECTION("negative lookahead")
        {
            FOONATHAN_LEX_PEG(!r("ab") + 'a');

            REQUIRE(verify<PEG>("abc", 0));
            REQUIRE(verify<PEG>("a", 1));
            REQUIRE(verify<PEG>("", 0));
        }
    }
    SECTION("convenience")
    {
        SECTION("minus")
        {
            // equivalent to 'a'
            FOONATHAN_LEX_PEG(minus(r('a') / 'b', 'b'));

            REQUIRE(verify<PEG>("a", 1));
            REQUIRE(verify<PEG>("ab", 1));
            REQUIRE(verify<PEG>("b", 0));
            REQUIRE(verify<PEG>("", 0));
        }
        SECTION("if_then_else")
        {
            FOONATHAN_LEX_PEG(if_then_else('a', 'b', 'c'));

            REQUIRE(verify<PEG>("abc", 2));
            REQUIRE(verify<PEG>("c", 1));
            REQUIRE(verify<PEG>("bc", 0));
        }
        SECTION("until")
        {
            FOONATHAN_LEX_PEG(until(' ', 'a'));

            REQUIRE(verify<PEG>("aaa b", 4));
            REQUIRE(verify<PEG>(" b", 1));
            REQUIRE(verify<PEG>("ab b", 0));
            REQUIRE(verify<PEG>("aaaaa", 0));
        }
        SECTION("until_excluding")
        {
            FOONATHAN_LEX_PEG(until_excluding(' ', 'a'));

            REQUIRE(verify<PEG>("aaa b", 3));
            REQUIRE(verify<PEG>(" b", 0));
            REQUIRE(verify<PEG>("ab b", 0));
            REQUIRE(verify<PEG>("aaaaa", 0));
        }
        SECTION("list")
        {
            FOONATHAN_LEX_PEG(list('a', ' '));

            REQUIRE(verify<PEG>("a a a", 5));
            REQUIRE(verify<PEG>("a a a ", 5));
            REQUIRE(verify<PEG>("a a aa", 5));
            REQUIRE(verify<PEG>("a", 1));
            REQUIRE(verify<PEG>("a ", 1));
            REQUIRE(verify<PEG>("b", 0));
        }
        SECTION("list_trailing")
        {
            FOONATHAN_LEX_PEG(list_trailing('a', ' '));

            REQUIRE(verify<PEG>("a a a", 5));
            REQUIRE(verify<PEG>("a a a ", 6));
            REQUIRE(verify<PEG>("a a aa", 5));
            REQUIRE(verify<PEG>("a", 1));
            REQUIRE(verify<PEG>("a ", 2));
            REQUIRE(verify<PEG>("b", 0));
        }
        SECTION("opt_padded")
        {
            FOONATHAN_LEX_PEG(opt_padded('l', 'a', 'r'));

            REQUIRE(verify<PEG>("a", 1));
            REQUIRE(verify<PEG>("lla", 3));
            REQUIRE(verify<PEG>("arr", 3));
            REQUIRE(verify<PEG>("larr", 4));
            REQUIRE(verify<PEG>("lrr", 0));
        }
        SECTION("padded")
        {
            FOONATHAN_LEX_PEG(padded('l', 'a', 'r'));

            REQUIRE(verify<PEG>("a", 0));
            REQUIRE(verify<PEG>("lla", 3));
            REQUIRE(verify<PEG>("arr", 3));
            REQUIRE(verify<PEG>("larr", 4));
            REQUIRE(verify<PEG>("lrr", 0));
        }
        SECTION("repeated")
        {
            FOONATHAN_LEX_PEG(repeated<2, 4>('a'));

            REQUIRE(verify<PEG>("a", 0));
            REQUIRE(verify<PEG>("aa", 2));
            REQUIRE(verify<PEG>("aaa", 3));
            REQUIRE(verify<PEG>("aaaa", 4));
            REQUIRE(verify<PEG>("aaaaa", 0));
        }
        SECTION("times")
        {
            FOONATHAN_LEX_PEG(times<3>('a'));

            REQUIRE(verify<PEG>("a", 0));
            REQUIRE(verify<PEG>("aa", 0));
            REQUIRE(verify<PEG>("aaa", 3));
            REQUIRE(verify<PEG>("aaaa", 0));
        }
        SECTION("at_most")
        {
            FOONATHAN_LEX_PEG(at_most<3>('a'));

            REQUIRE(verify<PEG>("a", 1));
            REQUIRE(verify<PEG>("aa", 2));
            REQUIRE(verify<PEG>("aaa", 3));
            REQUIRE(verify<PEG>("aaaa", 0));
        }
        SECTION("at_least")
        {
            FOONATHAN_LEX_PEG(at_least<2>('a'));

            REQUIRE(verify<PEG>("a", 0));
            REQUIRE(verify<PEG>("aa", 2));
            REQUIRE(verify<PEG>("aaa", 3));
            REQUIRE(verify<PEG>("aaaa", 4));
        }
    }
}
