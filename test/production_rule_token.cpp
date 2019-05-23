// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/lex/rule_production.hpp>

#include <catch.hpp>

#include "test.hpp"

namespace lex = foonathan::lex;

namespace
{
using test_spec = lex::token_spec<struct A, struct B, struct C>;
struct A : lex::literal_token<'a'>
{};
struct B : lex::literal_token<'b'>
{};
struct C : lex::literal_token<'c'>
{};

template <class TLP, typename Func, std::size_t N>
constexpr auto parse(Func&& f, const char (&str)[N])
{
    lex::tokenizer<test_spec> tokenizer(str);
    return TLP::parse(tokenizer, f);
}

void verify(const lex::parse_result<int>& result, int expected)
{
    if (expected == -1)
    {
        CHECK(!result.is_success());
    }
    else
    {
        REQUIRE(result.is_success());
        REQUIRE(result.value() == expected);
    }
}

#define FOONATHAN_LEX_P(Name, ...)                                                                 \
    struct Name : lex::rule_production<Name, grammar>                                              \
    {                                                                                              \
        static constexpr auto rule() noexcept                                                      \
        {                                                                                          \
            using namespace lex::production_rule;                                                  \
            return __VA_ARGS__;                                                                    \
        }                                                                                          \
    }
} // namespace

TEST_CASE("rule_production: token")
{
    using grammar = lex::grammar<test_spec, struct P>;
    FOONATHAN_LEX_P(P, A{});

    struct visitor
    {
        constexpr int production(P, lex::static_token<A>) const
        {
            return 0;
        }

        constexpr void error(lex::unexpected_token<grammar, P, A>,
                                  const lex::tokenizer<test_spec>&) const
        {}
    };

    FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "");
    verify(r0, -1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r1 = parse<P>(visitor{}, "a");
    verify(r1, 0);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "b");
    verify(r2, -1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "aab");
    verify(r3, 0);
}

TEST_CASE("rule_production: silent")
{
    using grammar = lex::grammar<test_spec, struct P>;
    FOONATHAN_LEX_P(P, silent<A> + B{} + silent(A{}));

    struct visitor
    {
        constexpr int production(P, lex::static_token<B>) const
        {
            return 0;
        }

        constexpr void error(lex::unexpected_token<grammar, P, A>,
                                  const lex::tokenizer<test_spec>&) const
        {}
        constexpr void error(lex::unexpected_token<grammar, P, B>,
                                  const lex::tokenizer<test_spec>&) const
        {}
    };

    FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "");
    verify(r0, -1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r1 = parse<P>(visitor{}, "a");
    verify(r1, -1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "b");
    verify(r2, -1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "aba");
    verify(r3, 0);
}

TEST_CASE("rule_production: eof")
{
    using grammar = lex::grammar<test_spec, struct P>;
    FOONATHAN_LEX_P(P, A{} + eof);

    struct visitor
    {
        constexpr int production(P, lex::static_token<A>) const
        {
            return 0;
        }

        constexpr void error(lex::unexpected_token<grammar, P, A>,
                                  const lex::tokenizer<test_spec>&) const
        {}
        constexpr void error(lex::unexpected_token<grammar, P, lex::eof_token>,
                                  const lex::tokenizer<test_spec>&) const
        {}
    };

    FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "");
    verify(r0, -1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r1 = parse<P>(visitor{}, "a");
    verify(r1, 0);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "b");
    verify(r2, -1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "aab");
    verify(r3, -1);
}

TEST_CASE("rule_production: token sequence")
{
    using grammar = lex::grammar<test_spec, struct P>;
    FOONATHAN_LEX_P(P, A{} + B{};);

    struct visitor
    {
        constexpr int production(P, lex::static_token<A>, lex::static_token<B>) const
        {
            return 0;
        }

        constexpr void error(lex::unexpected_token<grammar, P, A>,
                                  const lex::tokenizer<test_spec>&) const
        {}
        constexpr void error(lex::unexpected_token<grammar, P, B>,
                                  const lex::tokenizer<test_spec>&) const
        {}
    };

    FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "");
    verify(r0, -1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r1 = parse<P>(visitor{}, "a");
    verify(r1, -1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "b");
    verify(r2, -1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "ab");
    verify(r3, 0);
}

TEST_CASE("rule_production: token choice")
{
    using grammar = lex::grammar<test_spec, struct P>;
    FOONATHAN_LEX_P(P, A{} / B{};);

    struct visitor
    {
        constexpr int production(P, lex::static_token<A>) const
        {
            return 0;
        }
        constexpr int production(P, lex::static_token<B>) const
        {
            return 1;
        }

        constexpr void error(lex::exhausted_token_choice<grammar, P, A, B>,
                                  const lex::tokenizer<test_spec>&) const
        {}
    };

    FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "");
    verify(r0, -1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r1 = parse<P>(visitor{}, "a");
    verify(r1, 0);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "b");
    verify(r2, 1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "ab");
    verify(r3, 0);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r4 = parse<P>(visitor{}, "cab");
    verify(r4, -1);
}

TEST_CASE("rule_production: token opt")
{
    using grammar = lex::grammar<test_spec, struct P>;
    FOONATHAN_LEX_P(P, opt(A{}));

    struct visitor
    {
        constexpr int production(P) const
        {
            return 0;
        }
        constexpr int production(P, lex::static_token<A>) const
        {
            return 1;
        }
    };

    FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "");
    verify(r0, 0);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r1 = parse<P>(visitor{}, "a");
    verify(r1, 1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "b");
    verify(r2, 0);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "aab");
    verify(r3, 1);
}

TEST_CASE("rule_production: token complex")
{
    using grammar = lex::grammar<test_spec, struct P>;
    FOONATHAN_LEX_P(P, A{} / (B{} + B{} / C{}) + A{} + opt(B{}) + eof);

    struct visitor
    {
        constexpr int production(P, lex::static_token<A>, lex::static_token<A>,
                                 lex::static_token<B>) const
        {
            return 0;
        }
        constexpr int production(P, lex::static_token<A>, lex::static_token<A>) const
        {
            return 1;
        }

        constexpr int production(P, lex::static_token<B>, lex::static_token<B>,
                                 lex::static_token<A>, lex::static_token<B>) const
        {
            return 2;
        }
        constexpr int production(P, lex::static_token<B>, lex::static_token<B>,
                                 lex::static_token<A>) const
        {
            return 3;
        }

        constexpr int production(P, lex::static_token<B>, lex::static_token<C>,
                                 lex::static_token<A>, lex::static_token<B>) const
        {
            return 4;
        }
        constexpr int production(P, lex::static_token<B>, lex::static_token<C>,
                                 lex::static_token<A>) const
        {
            return 5;
        }

        constexpr void error(lex::unexpected_token<grammar, P, A>,
                                  const lex::tokenizer<test_spec>&) const
        {}
        constexpr void error(lex::unexpected_token<grammar, P, B>,
                                  const lex::tokenizer<test_spec>&) const
        {}
        constexpr void error(lex::unexpected_token<grammar, P, lex::eof_token>,
                                  const lex::tokenizer<test_spec>&) const
        {}

        constexpr void error(lex::exhausted_token_choice<grammar, P, A, B>,
                                  const lex::tokenizer<test_spec>&) const
        {}
        constexpr void error(lex::exhausted_token_choice<grammar, P, B, C>,
                                  const lex::tokenizer<test_spec>&) const
        {}
    };

    FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "aab");
    verify(r0, 0);
    FOONATHAN_LEX_TEST_CONSTEXPR auto r1 = parse<P>(visitor{}, "aa");
    verify(r1, 1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "bbab");
    verify(r2, 2);
    FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "bba");
    verify(r3, 3);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r4 = parse<P>(visitor{}, "bcab");
    verify(r4, 4);
    FOONATHAN_LEX_TEST_CONSTEXPR auto r5 = parse<P>(visitor{}, "bca");
    verify(r5, 5);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r6 = parse<P>(visitor{}, "a");
    verify(r6, -1);
    FOONATHAN_LEX_TEST_CONSTEXPR auto r7 = parse<P>(visitor{}, "bb");
    verify(r7, -1);
    FOONATHAN_LEX_TEST_CONSTEXPR auto r8 = parse<P>(visitor{}, "bcb");
    verify(r8, -1);
    FOONATHAN_LEX_TEST_CONSTEXPR auto r9 = parse<P>(visitor{}, "c");
    verify(r9, -1);
}
