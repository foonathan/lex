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
        REQUIRE(!result.is_success());
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

TEST_CASE("rule_production: production")
{
    using grammar = lex::grammar<test_spec, struct P, struct Q>;
    FOONATHAN_LEX_P(Q, B{});
    FOONATHAN_LEX_P(P, A{} + Q{} + C{});

    struct visitor
    {
        constexpr float production(Q, lex::static_token<B>) const
        {
            return 3.14f;
        }

        constexpr int production(P, lex::static_token<A>, float q, lex::static_token<C>) const
        {
            return 1 + int(q);
        }

        constexpr void error(lex::unexpected_token<grammar, P, A>,
                             const lex::tokenizer<test_spec>&) const
        {}
        constexpr void error(lex::unexpected_token<grammar, P, C>,
                             const lex::tokenizer<test_spec>&) const
        {}
        constexpr void error(lex::unexpected_token<grammar, Q, B>,
                             const lex::tokenizer<test_spec>&) const
        {}
    };

    FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "");
    verify(r0, -1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r1 = parse<P>(visitor{}, "b");
    verify(r1, -1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "abc");
    verify(r2, 4);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "ab");
    verify(r3, -1);
}

TEST_CASE("rule_production: choice")
{
    using grammar = lex::grammar<test_spec, struct P, struct Q1, struct Q2>;
    FOONATHAN_LEX_P(Q1, A{});
    FOONATHAN_LEX_P(Q2, B{});
    FOONATHAN_LEX_P(P, A{} >> Q1{} + C{} | B{} >> Q2{} + C{});

    struct visitor
    {
        constexpr int production(Q1, lex::static_token<A>) const
        {
            return 0;
        }
        constexpr int production(Q2, lex::static_token<B>) const
        {
            return 1;
        }

        constexpr int production(P, int value, lex::static_token<C>) const
        {
            return 10 + value;
        }

        constexpr void error(lex::unexpected_token<grammar, Q1, A>,
                             const lex::tokenizer<test_spec>&) const
        {}
        constexpr void error(lex::unexpected_token<grammar, Q2, B>,
                             const lex::tokenizer<test_spec>&) const
        {}
        constexpr void error(lex::unexpected_token<grammar, P, C>,
                             const lex::tokenizer<test_spec>&) const
        {}

        constexpr void error(lex::exhausted_choice<grammar, P>,
                             const lex::tokenizer<test_spec>&) const
        {}
    };

    FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "ac");
    verify(r0, 10);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r1 = parse<P>(visitor{}, "bc");
    verify(r1, 11);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "c");
    verify(r2, -1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "a");
    verify(r3, -1);
}

TEST_CASE("rule_production: choice with tokens")
{
    using grammar = lex::grammar<test_spec, struct P, struct Q1>;
    FOONATHAN_LEX_P(Q1, A{});
    FOONATHAN_LEX_P(P, A{} >> Q1{} + C{} | B{} + C{} | C{});

    struct visitor
    {
        constexpr int production(Q1, lex::static_token<A>) const
        {
            return 0;
        }

        constexpr int production(P, int value, lex::static_token<C>) const
        {
            return 10 + value;
        }
        constexpr int production(P, lex::static_token<B>, lex::static_token<C>) const
        {
            return 11;
        }
        constexpr int production(P, lex::static_token<C>) const
        {
            return 12;
        }

        constexpr void error(lex::unexpected_token<grammar, Q1, A>,
                             const lex::tokenizer<test_spec>&) const
        {}
        constexpr void error(lex::unexpected_token<grammar, P, B>,
                             const lex::tokenizer<test_spec>&) const
        {}
        constexpr void error(lex::unexpected_token<grammar, P, C>,
                             const lex::tokenizer<test_spec>&) const
        {}

        constexpr void error(lex::exhausted_choice<grammar, P>,
                             const lex::tokenizer<test_spec>&) const
        {}
    };

    FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "ac");
    verify(r0, 10);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r1 = parse<P>(visitor{}, "bc");
    verify(r1, 11);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "c");
    verify(r2, 12);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "a");
    verify(r3, -1);
}

TEST_CASE("rule_production: choice with complex peek")
{
    using grammar = lex::grammar<test_spec, struct P, struct Q>;
    FOONATHAN_LEX_P(Q, A{} + C{});
    FOONATHAN_LEX_P(P, A{} + B{} >> A{} + B{} + C{} | Q{} >> A{} + C{} | A{});

    struct visitor
    {
        constexpr int production(P, lex::static_token<A>, lex::static_token<B>,
                                 lex::static_token<C>) const
        {
            return 1;
        }
        constexpr int production(P, lex::static_token<A>, lex::static_token<C>) const
        {
            return 2;
        }
        constexpr int production(P, lex::static_token<A>) const
        {
            return 3;
        }

        constexpr void error(lex::unexpected_token<grammar, P, A>,
                             const lex::tokenizer<test_spec>&) const
        {}
        constexpr void error(lex::unexpected_token<grammar, P, B>,
                             const lex::tokenizer<test_spec>&) const
        {}
        constexpr void error(lex::unexpected_token<grammar, P, C>,
                             const lex::tokenizer<test_spec>&) const
        {}

        constexpr void error(lex::exhausted_choice<grammar, P>,
                             const lex::tokenizer<test_spec>&) const
        {}
    };

    FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "abc");
    verify(r0, 1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r1 = parse<P>(visitor{}, "ac");
    verify(r1, 2);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "a");
    verify(r2, 3);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "ab");
    verify(r3, -1);
}

TEST_CASE("rule_production: right recursion")
{
    using grammar = lex::grammar<test_spec, struct P>;
    FOONATHAN_LEX_P(P, A{} >> A{} + P{} | B{});

    struct visitor
    {
        int           result_of(P);
        constexpr int production(P, lex::static_token<B>) const
        {
            return 0;
        }
        constexpr int production(P, lex::static_token<A>, int value) const
        {
            return 1 + value;
        }

        constexpr void error(lex::unexpected_token<grammar, P, A>,
                             const lex::tokenizer<test_spec>&) const
        {}
        constexpr void error(lex::unexpected_token<grammar, P, B>,
                             const lex::tokenizer<test_spec>&) const
        {}

        constexpr void error(lex::exhausted_choice<grammar, P>,
                             const lex::tokenizer<test_spec>&) const
        {}
    };

    FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "");
    verify(r0, -1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r1 = parse<P>(visitor{}, "b");
    verify(r1, 0);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "ab");
    verify(r2, 1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "aab");
    verify(r3, 2);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r4 = parse<P>(visitor{}, "aaab");
    verify(r4, 3);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r5 = parse<P>(visitor{}, "a");
    verify(r5, -1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r6 = parse<P>(visitor{}, "ac");
    verify(r6, -1);
}

TEST_CASE("rule_production: middle recursion")
{
    using grammar = lex::grammar<test_spec, struct P>;
    FOONATHAN_LEX_P(P, A{} >> A{} + P{} + A{} | B{});

    struct visitor
    {
        int           result_of(P);
        constexpr int production(P, lex::static_token<B>) const
        {
            return 0;
        }
        constexpr int production(P, lex::static_token<A>, int value, lex::static_token<A>) const
        {
            return 1 + value;
        }

        constexpr void error(lex::unexpected_token<grammar, P, A>,
                             const lex::tokenizer<test_spec>&) const
        {}
        constexpr void error(lex::unexpected_token<grammar, P, B>,
                             const lex::tokenizer<test_spec>&) const
        {}

        constexpr void error(lex::exhausted_choice<grammar, P>,
                             const lex::tokenizer<test_spec>&) const
        {}
    };

    FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "");
    verify(r0, -1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r1 = parse<P>(visitor{}, "b");
    verify(r1, 0);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "aba");
    verify(r2, 1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "aabaa");
    verify(r3, 2);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r4 = parse<P>(visitor{}, "aaabaaa");
    verify(r4, 3);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r5 = parse<P>(visitor{}, "a");
    verify(r5, -1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r6 = parse<P>(visitor{}, "ac");
    verify(r6, -1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r7 = parse<P>(visitor{}, "ab");
    verify(r7, -1);
}

TEST_CASE("rule_production: left recursion")
{
    using grammar = lex::grammar<test_spec, struct P>;
    FOONATHAN_LEX_P(P, B{} | else_ >> P{} + A{});

    struct visitor
    {
        int           result_of(P);
        constexpr int production(P, lex::static_token<B>) const
        {
            return 0;
        }
        constexpr int production(P, int value, lex::static_token<A>) const
        {
            return 1 + value;
        }

        constexpr void error(lex::unexpected_token<grammar, P, A>,
                             const lex::tokenizer<test_spec>&) const
        {}
        constexpr void error(lex::unexpected_token<grammar, P, B>,
                             const lex::tokenizer<test_spec>&) const
        {}

        constexpr void error(lex::exhausted_choice<grammar, P>,
                             const lex::tokenizer<test_spec>&) const
        {}
    };

    FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "");
    verify(r0, -1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r1 = parse<P>(visitor{}, "b");
    verify(r1, 0);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "ba");
    verify(r2, 1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "baa");
    verify(r3, 2);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r4 = parse<P>(visitor{}, "baaa");
    verify(r4, 3);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r5 = parse<P>(visitor{}, "a");
    verify(r5, -1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r6 = parse<P>(visitor{}, "ca");
    verify(r6, -1);
}

TEST_CASE("rule_production: indirect recursion")
{
    using grammar = lex::grammar<test_spec, struct P, struct Q>;
    FOONATHAN_LEX_P(Q, B{} >> B{} + recurse<P> | C{});
    FOONATHAN_LEX_P(P, A{} + Q{});

    struct visitor
    {
        constexpr int production(Q, lex::static_token<B>, int value) const
        {
            return 1 + value;
        }
        constexpr int production(Q, lex::static_token<C>) const
        {
            return 0;
        }

        int           result_of(P);
        constexpr int production(P, lex::static_token<A>, int value) const
        {
            return 10 * value;
        }

        constexpr void error(lex::unexpected_token<grammar, Q, B>,
                             const lex::tokenizer<test_spec>&) const
        {}
        constexpr void error(lex::unexpected_token<grammar, Q, C>,
                             const lex::tokenizer<test_spec>&) const
        {}
        constexpr void error(lex::unexpected_token<grammar, P, A>,
                             const lex::tokenizer<test_spec>&) const
        {}

        constexpr void error(lex::exhausted_choice<grammar, Q>,
                             const lex::tokenizer<test_spec>&) const
        {}
    };

    FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "");
    verify(r0, -1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r1 = parse<P>(visitor{}, "ac");
    verify(r1, 0);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "abac");
    verify(r2, 10);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "ababac");
    verify(r3, 110);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r4 = parse<P>(visitor{}, "abababac");
    verify(r4, 1110);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r5 = parse<P>(visitor{}, "ab");
    verify(r5, -1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r6 = parse<P>(visitor{}, "abc");
    verify(r6, -1);
}

TEST_CASE("rule_production: finish")
{
    using grammar = lex::grammar<test_spec, struct P>;
    FOONATHAN_LEX_P(P, A{} >> A{} + P{} | B{});

    struct visitor
    {
        unsigned           result_of(P);
        constexpr unsigned production(P, lex::static_token<B>) const
        {
            return 0;
        }
        constexpr unsigned production(P, lex::static_token<A>, unsigned value) const
        {
            return 1 + value;
        }
        constexpr int finish(P, unsigned value) const
        {
            return -static_cast<int>(value);
        }

        constexpr void error(lex::unexpected_token<grammar, P, A>,
                             const lex::tokenizer<test_spec>&) const
        {}
        constexpr void error(lex::unexpected_token<grammar, P, B>,
                             const lex::tokenizer<test_spec>&) const
        {}

        constexpr void error(lex::exhausted_choice<grammar, P>,
                             const lex::tokenizer<test_spec>&) const
        {}
    };

    FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "b");
    verify(r0, 0);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r1 = parse<P>(visitor{}, "aab");
    verify(r1, -2);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "aaab");
    verify(r2, -3);
}

TEST_CASE("rule_production: finish left recursion")
{
    using grammar = lex::grammar<test_spec, struct P>;
    FOONATHAN_LEX_P(P, B{} | else_ >> P{} + A{});

    struct visitor
    {
        unsigned           result_of(P);
        constexpr unsigned production(P, lex::static_token<B>) const
        {
            return 0;
        }
        constexpr unsigned production(P, unsigned value, lex::static_token<A>) const
        {
            return 1 + value;
        }
        constexpr int finish(P, unsigned value) const
        {
            return -static_cast<int>(value);
        }

        constexpr void error(lex::unexpected_token<grammar, P, A>,
                             const lex::tokenizer<test_spec>&) const
        {}
        constexpr void error(lex::unexpected_token<grammar, P, B>,
                             const lex::tokenizer<test_spec>&) const
        {}

        constexpr void error(lex::exhausted_choice<grammar, P>,
                             const lex::tokenizer<test_spec>&) const
        {}
    };

    FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "b");
    verify(r0, 0);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r1 = parse<P>(visitor{}, "baa");
    verify(r1, -2);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "baaa");
    verify(r2, -3);
}
