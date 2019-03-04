// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/lex/rule_production.hpp>

#include <catch.hpp>

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

void verify(lex::parse_result<int> result, int expected)
{
    if (expected == -1)
    {
        CHECK(!result.is_success());
    }
    else
    {
        CHECK(result.is_success());
        CHECK(result.value() == expected);
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
        constexpr float operator()(Q, lex::static_token<B>) const
        {
            return 3.14f;
        }

        constexpr int operator()(P, lex::static_token<A>, float q, lex::static_token<C>) const
        {
            return 1 + int(q);
        }

        constexpr void operator()(lex::unexpected_token<grammar, P, A>,
                                  const lex::tokenizer<test_spec>&) const
        {}
        constexpr void operator()(lex::unexpected_token<grammar, P, C>,
                                  const lex::tokenizer<test_spec>&) const
        {}
        constexpr void operator()(lex::unexpected_token<grammar, Q, B>,
                                  const lex::tokenizer<test_spec>&) const
        {}
    };

    constexpr auto r0 = parse<P>(visitor{}, "");
    verify(r0, -1);

    constexpr auto r1 = parse<P>(visitor{}, "b");
    verify(r1, -1);

    constexpr auto r2 = parse<P>(visitor{}, "abc");
    verify(r2, 4);

    constexpr auto r3 = parse<P>(visitor{}, "ab");
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
        constexpr int operator()(Q1, lex::static_token<A>) const
        {
            return 0;
        }
        constexpr int operator()(Q2, lex::static_token<B>) const
        {
            return 1;
        }

        constexpr int operator()(P, int value, lex::static_token<C>) const
        {
            return 10 + value;
        }

        constexpr void operator()(lex::unexpected_token<grammar, Q1, A>,
                                  const lex::tokenizer<test_spec>&) const
        {}
        constexpr void operator()(lex::unexpected_token<grammar, Q2, B>,
                                  const lex::tokenizer<test_spec>&) const
        {}
        constexpr void operator()(lex::unexpected_token<grammar, P, C>,
                                  const lex::tokenizer<test_spec>&) const
        {}

        constexpr void operator()(lex::exhausted_token_choice<grammar, P, A, B>,
                                  const lex::tokenizer<test_spec>&) const
        {}
    };

    constexpr auto r0 = parse<P>(visitor{}, "ac");
    verify(r0, 10);

    constexpr auto r1 = parse<P>(visitor{}, "bc");
    verify(r1, 11);

    constexpr auto r2 = parse<P>(visitor{}, "c");
    verify(r2, -1);

    constexpr auto r3 = parse<P>(visitor{}, "a");
    verify(r3, -1);
}
