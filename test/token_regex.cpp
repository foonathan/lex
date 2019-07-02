// Copyright (C) 2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/lex/token_regex.hpp>

#include <doctest.h>
#include <set>
#include <string>

#include <foonathan/lex/tokenizer.hpp>

namespace lex = foonathan::lex;

namespace
{
struct spec : lex::token_spec<struct a, struct b, struct c>
{};
struct a : lex::literal_token<'a'>
{};
struct b : lex::literal_token<'b'>
{};
struct c : lex::literal_token<'c'>
{};

template <class Regex>
constexpr bool match_impl(const char* str, std::size_t n, Regex regex)
{
    lex::tokenizer<spec> tokenizer(str, str + n);
    return lex::regex_match(tokenizer, regex);
}

template <class Regex, class... Matches>
void verify(const char* desc, Regex regex, Matches... matches_)
{
    std::set<std::string> matches{matches_...};

    auto actual_desc = lex::regex_description(regex);
    if (desc)
        CHECK(std::string(actual_desc) == std::string(desc));
    INFO("description: " << actual_desc);

    constexpr auto empty = match_impl("", 0, Regex{});
    CHECK(empty == matches.count("") > 0);

    for (auto str : {"a",   "b",   "c",   "aa",  "ab",  "ac",  "ba",  "bb",  "bc",  "ca",
                     "cb",  "cc",  "aaa", "aab", "aac", "aba", "abb", "abc", "aca", "acb",
                     "acc", "baa", "bab", "bac", "bba", "bbb", "bbc", "bca", "bcb", "bcc",
                     "caa", "cab", "cac", "cba", "cbb", "cbc", "cca", "ccb", "ccc"})
    {
        INFO("input: " << str);
        CHECK(match_impl(str, std::strlen(str), regex) == matches.count(str) > 0);
        matches.erase(str);
    }

    // check longer strings
    for (auto m : matches)
    {
        INFO("input: " << m);
        CHECK(match_impl(m.c_str(), m.size(), regex));
    }
}
template <class Regex, class... Matches>
void verify_all(const char* desc, Regex regex)
{
    auto actual_desc = lex::regex_description(regex);
    if (desc)
        CHECK(std::string(actual_desc) == std::string(desc));
    INFO("description: " << actual_desc);

    constexpr auto empty = match_impl("", 0, Regex{});
    CHECK(empty);

    for (auto str : {"a",   "b",   "c",   "aa",  "ab",  "ac",  "ba",  "bb",  "bc",  "ca",
                     "cb",  "cc",  "aaa", "aab", "aac", "aba", "abb", "abc", "aca", "acb",
                     "acc", "baa", "bab", "bac", "bba", "bbb", "bbc", "bca", "bcb", "bcc",
                     "caa", "cab", "cac", "cba", "cbb", "cbc", "cca", "ccb", "ccc"})
    {
        INFO("input: " << str);
        CHECK(match_impl(str, std::strlen(str), regex));
    }
}

template <class A, class B, class... Matches>
void verify_choice(const char* description, A a, B b, Matches... matches)
{
    INFO(description);
    verify(description, a / b, matches...);
    verify(nullptr, b / a, matches...);
}
} // namespace

TEST_CASE("token_regex")
{
    // atom
    verify("a", a{}, "a");
    verify("b", b{}, "b");
    verify("c", c{}, "c");

    // sequence: simple
    verify("ab", a{} + b{}, "ab");
    verify("abc", a{} + b{} + c{}, "abc");
    verify("abc", (a{} + b{}) + c{}, "abc");

    // sequence: with choice
    verify("ab|bb", a{} / b{} + b{}, "ab", "bb");
    verify("a(b|c)", a{} + b{} / c{}, "ab", "ac");

    // star: simple
    verify("(a)*", star(a{}), "", "a", "aa", "aaa", "aaaa");
    verify("(ab)*", star(a{} + b{}), "", "ab", "abab", "ababab");

    // star: with choice
    verify("(a|b)*", star(a{} / b{}), "", "a", "aa", "aaa", "b", "bb", "bbb", "ab", "ba", "aab",
           "aba", "abb", "bab", "baa", "bba");
    verify("(a|b)*", star(a{} / b{} / a{}), "", "a", "aa", "aaa", "b", "bb", "bbb", "ab", "ba",
           "aab", "aba", "abb", "bab", "baa", "bba");
    verify_all("(a|b|c)*", star(a{} / b{} / c{}));
    verify("a(a|b)*", a{} + star(a{} / b{} / a{}), "a", "aa", "aaa", "ab", "aab", "aba", "abb");

    // star: suffix
    verify("(a|b)*c", star(a{} / b{}) + c{}, "c", "ac", "aac", "bc", "bbc", "abc", "bac");
    verify("c(a|b)*c", c{} + star(a{} / b{}) + c{}, "cc", "cac", "cbc", "cbbc", "cabc", "cbac");

    // choice: simple
    verify("a|b", a{} / b{}, "a", "b");
    verify("b|a", b{} / a{}, "a", "b");
    verify("a|b|c", a{} / b{} / c{}, "a", "b", "c");
    verify("a|b|c", (a{} / b{}) / c{}, "a", "b", "c");

    // choice: redundancy
    verify("a|b", a{} / a{} / b{}, "a", "b");
    verify("a|b", a{} / b{} / a{}, "a", "b");

    // choice: atom / sequence
    verify_choice("a(b)?", a{}, a{} + b{}, "a", "ab");
    verify_choice("a|bc", a{}, b{} + c{}, "a", "bc");

    // choice: atom / choice
    verify_choice("a|b", a{}, a{} / b{}, "a", "b");
    verify_choice("a|b", a{}, b{} / a{}, "a", "b");
    verify_choice("a|b|c", a{}, a{} / b{} / c{}, "a", "b", "c");
    verify_choice("a|c|b", a{}, b{} / c{} / a{}, "a", "b", "c");

    // choice: atom / sequence with choice
    verify_choice("a(c)?|bc", a{}, (a{} / b{}) + c{}, "a", "ac", "bc");
    verify_choice("a|bc|cc", a{}, (b{} / c{}) + c{}, "a", "bc", "cc");
    verify_choice("a(c)?|cc|bc", a{}, (b{} / c{} / a{}) + c{}, "a", "ac", "bc", "cc");

    // choice: atom / choice with sequence
    verify_choice("a(b)?|b", a{}, (a{} + b{}) / b{}, "a", "ab", "b");
    verify_choice("a|b(a)?", a{}, (b{} + a{}) / b{}, "a", "ba", "b");

    // choice: sequence / sequence
    verify_choice("a(b|c)", a{} + b{}, a{} + c{}, "ab", "ac");
    verify_choice("ab|bc", a{} + b{}, b{} + c{}, "ab", "bc");

    // choice: sequence / sequence with choice
    verify_choice("aa|ca|ba", a{} / b{} + a{}, a{} / c{} + a{}, "aa", "ba", "ca");
    verify_choice("aa|ba|cb", a{} / b{} + a{}, c{} + b{}, "aa", "ba", "cb");

    // opt - doesn't need to be that exhaustive, it is a choice after all
    verify("(a)?", opt(a{}), "", "a");
    verify("c(c|abc)", c{} + opt(a{} + b{}) + c{}, "cabc", "cc");
    verify("(a|b)?", opt(a{} / b{}) / a{}, "", "a", "b");

    // plus - doesn't need to be that exhaustive, it is a sequence after all
    verify("a(a)*", plus(a{}), "a", "aa", "aaa");
    verify("ab(ab)*", plus(a{} + b{}), "ab", "abab");
    verify("a(a|b)*|b(a|b)*", plus(a{} / b{}), "a", "b", "aa", "ab", "ba", "bb", "aab", "aaa",
           "aba", "abb", "baa", "bab", "bba", "bbb");
}
