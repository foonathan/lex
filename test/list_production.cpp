// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/lex/list_production.hpp>

#include <catch.hpp>

#include "test.hpp"

namespace lex = foonathan::lex;

namespace
{
using test_spec = lex::token_spec<struct comma, struct a, struct open, struct close>;

struct comma : lex::literal_token<','>
{};
struct a : lex::literal_token<'a'>
{};
struct open : lex::literal_token<'('>
{};
struct close : lex::literal_token<')'>
{};

template <class TLP, typename Func, std::size_t N>
constexpr auto parse(Func&& f, const char (&str)[N])
{
    lex::tokenizer<test_spec> tokenizer(str);
    return TLP::parse(tokenizer, f);
}

constexpr struct unmatched_t
{
} unmatched;

void verify(const lex::parse_result<int>& result, unmatched_t)
{
    REQUIRE(result.is_unmatched());
}

void verify(const lex::parse_result<int>& result, int expected)
{
    REQUIRE(result.is_success());
    REQUIRE(result.value() == expected);
}
} // namespace

TEST_CASE("list_production: no separator, non-empty")
{
    using grammar = lex::grammar<test_spec, struct P>;
    struct P : lex::list_production<P, grammar>
    {
        using element   = a;
        using end_token = lex::eof_token;
    };

    struct visitor
    {
        constexpr int operator()(P, a)
        {
            return 1;
        }
        constexpr int operator()(P, int list, a)
        {
            return list + 1;
        }

        constexpr void operator()(lex::unexpected_token<grammar, P, a>,
                                  const lex::tokenizer<test_spec>&)
        {}
    };

    FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "");
    verify(r0, unmatched);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r1 = parse<P>(visitor{}, "a");
    verify(r1, 1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "aa");
    verify(r2, 2);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "aaa");
    verify(r3, 3);
}

TEST_CASE("list_production: no separator, empty")
{
    using grammar = lex::grammar<test_spec, struct P>;
    struct P : lex::list_production<P, grammar>
    {
        using element     = a;
        using end_token   = lex::eof_token;
        using allow_empty = std::true_type;
    };

    struct visitor
    {
        constexpr int operator()(P)
        {
            return 0;
        }
        constexpr int operator()(P, int list, a)
        {
            return list + 1;
        }

        constexpr void operator()(lex::unexpected_token<grammar, P, a>,
                                  const lex::tokenizer<test_spec>&)
        {}
    };

    FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "");
    verify(r0, 0);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r1 = parse<P>(visitor{}, "a");
    verify(r1, 1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "aa");
    verify(r2, 2);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "aaa");
    verify(r3, 3);
}

TEST_CASE("list_production: non-empty, non-trailing")
{
    using grammar = lex::grammar<test_spec, struct P>;
    struct P : lex::list_production<P, grammar>
    {
        using element         = a;
        using separator_token = comma;
    };

    struct visitor
    {
        constexpr int operator()(P, a)
        {
            return 1;
        }
        constexpr int operator()(P, int list, a)
        {
            return list + 1;
        }

        constexpr void operator()(lex::unexpected_token<grammar, P, a>,
                                  const lex::tokenizer<test_spec>&)
        {}
    };

    FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "");
    verify(r0, unmatched);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r1 = parse<P>(visitor{}, "a");
    verify(r1, 1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "a,a");
    verify(r2, 2);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "a,a,a");
    verify(r3, 3);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r4 = parse<P>(visitor{}, "a,");
    verify(r4, unmatched);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r5 = parse<P>(visitor{}, ",a");
    verify(r5, unmatched);
}

TEST_CASE("list_production: non-empty, trailing")
{
    using grammar = lex::grammar<test_spec, struct P>;
    struct P : lex::list_production<P, grammar>
    {
        using element         = a;
        using separator_token = comma;
        using end_token       = lex::eof_token;
        using allow_trailing  = std::true_type;
    };

    struct visitor
    {
        constexpr int operator()(P, a)
        {
            return 1;
        }
        constexpr int operator()(P, int list, a)
        {
            return list + 1;
        }

        constexpr void operator()(lex::unexpected_token<grammar, P, a>,
                                  const lex::tokenizer<test_spec>&)
        {}
    };

    FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "");
    verify(r0, unmatched);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r1 = parse<P>(visitor{}, "a");
    verify(r1, 1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "a,a");
    verify(r2, 2);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "a,a,a");
    verify(r3, 3);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r4 = parse<P>(visitor{}, "a,");
    verify(r4, 1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r5 = parse<P>(visitor{}, ",a");
    verify(r5, unmatched);
}

TEST_CASE("list_production: empty, non-trailing")
{
    using grammar = lex::grammar<test_spec, struct P>;
    struct P : lex::list_production<P, grammar>
    {
        using element         = a;
        using separator_token = comma;
        using end_token       = lex::eof_token;
        using allow_empty     = std::true_type;
    };

    struct visitor
    {
        constexpr int operator()(P)
        {
            return 0;
        }
        constexpr int operator()(P, int list, a)
        {
            return list + 1;
        }

        constexpr void operator()(lex::unexpected_token<grammar, P, a>,
                                  const lex::tokenizer<test_spec>&)
        {}
    };

    FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "");
    verify(r0, 0);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r1 = parse<P>(visitor{}, "a");
    verify(r1, 1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "a,a");
    verify(r2, 2);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "a,a,a");
    verify(r3, 3);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r4 = parse<P>(visitor{}, "a,");
    verify(r4, unmatched);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r5 = parse<P>(visitor{}, ",a");
    verify(r5, unmatched);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r6 = parse<P>(visitor{}, ",");
    verify(r6, unmatched);
}

TEST_CASE("list_production: empty, trailing")
{
    using grammar = lex::grammar<test_spec, struct P>;
    struct P : lex::list_production<P, grammar>
    {
        using element         = a;
        using separator_token = comma;
        using end_token       = lex::eof_token;
        using allow_empty     = std::true_type;
        using allow_trailing  = std::true_type;
    };

    struct visitor
    {
        constexpr int operator()(P)
        {
            return 0;
        }
        constexpr int operator()(P, int list, a)
        {
            return list + 1;
        }

        constexpr void operator()(lex::unexpected_token<grammar, P, a>,
                                  const lex::tokenizer<test_spec>&)
        {}
    };

    FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "");
    verify(r0, 0);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r1 = parse<P>(visitor{}, "a");
    verify(r1, 1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "a,a");
    verify(r2, 2);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "a,a,a");
    verify(r3, 3);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r4 = parse<P>(visitor{}, "a,");
    verify(r4, 1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r5 = parse<P>(visitor{}, ",a");
    verify(r5, unmatched);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r6 = parse<P>(visitor{}, ",");
    verify(r6, unmatched);
}

TEST_CASE("bracketed_list_production: no separator, non-empty")
{
    using grammar = lex::grammar<test_spec, struct P>;
    struct P : lex::bracketed_list_production<P, grammar>
    {
        using element       = a;
        using open_bracket  = open;
        using close_bracket = close;
    };

    struct visitor
    {
        constexpr int operator()(P, a)
        {
            return 1;
        }
        constexpr int operator()(P, int list, a)
        {
            return list + 1;
        }

        constexpr void operator()(lex::unexpected_token<grammar, P, a>,
                                  const lex::tokenizer<test_spec>&)
        {}

        constexpr void operator()(lex::unexpected_token<grammar, P, open>,
                                  const lex::tokenizer<test_spec>&)
        {}
        constexpr void operator()(lex::unexpected_token<grammar, P, close>,
                                  const lex::tokenizer<test_spec>&)
        {}
    };

    FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "()");
    verify(r0, unmatched);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r1 = parse<P>(visitor{}, "(a)");
    verify(r1, 1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "(aa)");
    verify(r2, 2);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "(aaa)");
    verify(r3, 3);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r4 = parse<P>(visitor{}, "aa");
    verify(r4, unmatched);
}

TEST_CASE("bracketed_list_production: no separator, empty")
{
    using grammar = lex::grammar<test_spec, struct P>;
    struct P : lex::bracketed_list_production<P, grammar>
    {
        using element       = a;
        using open_bracket  = open;
        using close_bracket = close;
        using allow_empty   = std::true_type;
    };

    struct visitor
    {
        constexpr int operator()(P)
        {
            return 0;
        }
        constexpr int operator()(P, int list, a)
        {
            return list + 1;
        }

        constexpr void operator()(lex::unexpected_token<grammar, P, a>,
                                  const lex::tokenizer<test_spec>&)
        {}

        constexpr void operator()(lex::unexpected_token<grammar, P, open>,
                                  const lex::tokenizer<test_spec>&)
        {}
        constexpr void operator()(lex::unexpected_token<grammar, P, close>,
                                  const lex::tokenizer<test_spec>&)
        {}
    };

    FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "()");
    verify(r0, 0);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r1 = parse<P>(visitor{}, "(a)");
    verify(r1, 1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "(aa)");
    verify(r2, 2);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "(aaa)");
    verify(r3, 3);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r4 = parse<P>(visitor{}, "aa");
    verify(r4, unmatched);
}

TEST_CASE("bracketed_list_production: non-empty, non-trailing")
{
    using grammar = lex::grammar<test_spec, struct P>;
    struct P : lex::bracketed_list_production<P, grammar>
    {
        using element         = a;
        using separator_token = comma;
        using open_bracket    = open;
        using close_bracket   = close;
    };

    struct visitor
    {
        constexpr int operator()(P, a)
        {
            return 1;
        }
        constexpr int operator()(P, int list, a)
        {
            return list + 1;
        }

        constexpr void operator()(lex::unexpected_token<grammar, P, a>,
                                  const lex::tokenizer<test_spec>&)
        {}

        constexpr void operator()(lex::unexpected_token<grammar, P, open>,
                                  const lex::tokenizer<test_spec>&)
        {}
        constexpr void operator()(lex::unexpected_token<grammar, P, close>,
                                  const lex::tokenizer<test_spec>&)
        {}
    };

    FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "()");
    verify(r0, unmatched);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r1 = parse<P>(visitor{}, "(a)");
    verify(r1, 1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "(a,a)");
    verify(r2, 2);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "(a,a,a)");
    verify(r3, 3);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r4 = parse<P>(visitor{}, "(a,)");
    verify(r4, unmatched);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r5 = parse<P>(visitor{}, "(,a)");
    verify(r5, unmatched);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r6 = parse<P>(visitor{}, "a,a");
    verify(r6, unmatched);
}

TEST_CASE("bracketed_list_production: non-empty, trailing")
{
    using grammar = lex::grammar<test_spec, struct P>;
    struct P : lex::bracketed_list_production<P, grammar>
    {
        using element         = a;
        using separator_token = comma;
        using open_bracket    = open;
        using close_bracket   = close;
        using allow_trailing  = std::true_type;
    };

    struct visitor
    {
        constexpr int operator()(P, a)
        {
            return 1;
        }
        constexpr int operator()(P, int list, a)
        {
            return list + 1;
        }

        constexpr void operator()(lex::unexpected_token<grammar, P, a>,
                                  const lex::tokenizer<test_spec>&)
        {}

        constexpr void operator()(lex::unexpected_token<grammar, P, open>,
                                  const lex::tokenizer<test_spec>&)
        {}
        constexpr void operator()(lex::unexpected_token<grammar, P, close>,
                                  const lex::tokenizer<test_spec>&)
        {}
    };

    FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "()");
    verify(r0, unmatched);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r1 = parse<P>(visitor{}, "(a)");
    verify(r1, 1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "(a,a)");
    verify(r2, 2);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "(a,a,a)");
    verify(r3, 3);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r4 = parse<P>(visitor{}, "(a,)");
    verify(r4, 1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r5 = parse<P>(visitor{}, "(,a)");
    verify(r5, unmatched);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r6 = parse<P>(visitor{}, "a,a");
    verify(r6, unmatched);
}

TEST_CASE("bracketed_list_production: empty, non-trailing")
{
    using grammar = lex::grammar<test_spec, struct P>;
    struct P : lex::bracketed_list_production<P, grammar>
    {
        using element         = a;
        using separator_token = comma;
        using open_bracket    = open;
        using close_bracket   = close;
        using allow_empty     = std::true_type;
    };

    struct visitor
    {
        constexpr int operator()(P)
        {
            return 0;
        }
        constexpr int operator()(P, int list, a)
        {
            return list + 1;
        }

        constexpr void operator()(lex::unexpected_token<grammar, P, a>,
                                  const lex::tokenizer<test_spec>&)
        {}

        constexpr void operator()(lex::unexpected_token<grammar, P, open>,
                                  const lex::tokenizer<test_spec>&)
        {}
        constexpr void operator()(lex::unexpected_token<grammar, P, close>,
                                  const lex::tokenizer<test_spec>&)
        {}
    };

    FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "()");
    verify(r0, 0);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r1 = parse<P>(visitor{}, "(a)");
    verify(r1, 1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "(a,a)");
    verify(r2, 2);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "(a,a,a)");
    verify(r3, 3);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r4 = parse<P>(visitor{}, "(a,)");
    verify(r4, unmatched);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r5 = parse<P>(visitor{}, "(,a)");
    verify(r5, unmatched);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r6 = parse<P>(visitor{}, "a,a");
    verify(r6, unmatched);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r7 = parse<P>(visitor{}, "(,)");
    verify(r7, unmatched);
}

TEST_CASE("bracketed_list_production: empty, trailing")
{
    using grammar = lex::grammar<test_spec, struct P>;
    struct P : lex::bracketed_list_production<P, grammar>
    {
        using element         = a;
        using separator_token = comma;
        using open_bracket    = open;
        using close_bracket   = close;
        using allow_empty     = std::true_type;
        using allow_trailing  = std::true_type;
    };

    struct visitor
    {
        constexpr int operator()(P)
        {
            return 0;
        }
        constexpr int operator()(P, int list, a)
        {
            return list + 1;
        }

        constexpr void operator()(lex::unexpected_token<grammar, P, a>,
                                  const lex::tokenizer<test_spec>&)
        {}

        constexpr void operator()(lex::unexpected_token<grammar, P, open>,
                                  const lex::tokenizer<test_spec>&)
        {}
        constexpr void operator()(lex::unexpected_token<grammar, P, close>,
                                  const lex::tokenizer<test_spec>&)
        {}
    };

    FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "()");
    verify(r0, 0);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r1 = parse<P>(visitor{}, "(a)");
    verify(r1, 1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "(a,a)");
    verify(r2, 2);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "(a,a,a)");
    verify(r3, 3);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r4 = parse<P>(visitor{}, "(a,)");
    verify(r4, 1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r5 = parse<P>(visitor{}, "(,a)");
    verify(r5, unmatched);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r6 = parse<P>(visitor{}, "a,a");
    verify(r6, unmatched);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r7 = parse<P>(visitor{}, "(,)");
    verify(r7, unmatched);
}
