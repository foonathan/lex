// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/lex/operator_production.hpp>

#include <catch.hpp>

#include <foonathan/lex/ascii.hpp>
#include <foonathan/lex/rule_production.hpp>

namespace lex = foonathan::lex;

namespace
{
using test_spec
    = lex::token_spec<struct whitespace, struct number, struct plus, struct minus, struct star>;

struct whitespace : lex::rule_token<whitespace, test_spec>, lex::whitespace_token
{
    static constexpr auto rule()
    {
        return lex::token_rule::star(lex::ascii::is_space);
    }
};

struct number : lex::rule_token<number, test_spec>
{
    static constexpr auto rule()
    {
        return lex::ascii::is_digit;
    }

    static constexpr int parse(lex::static_token<number> token)
    {
        return token.spelling()[0] - '0';
    }
};

struct plus : lex::literal_token<'+'>
{};
struct minus : lex::literal_token<'-'>
{};
struct star : lex::literal_token<'*'>
{};

template <class TLP, typename Func, std::size_t N>
constexpr auto parse(Func&& f, const char (&str)[N])
{
    lex::tokenizer<test_spec> tokenizer(str);
    return TLP::parse(tokenizer, f);
}

void verify(lex::parse_result<int> result, int expected)
{
    if (expected == 0)
    {
        REQUIRE(!result.is_success());
    }
    else
    {
        REQUIRE(result.is_success());
        REQUIRE(result.value() == expected);
    }
}

} // namespace

TEST_CASE("operator_production: bin_op_single")
{
    namespace r   = lex::operator_rule;
    using grammar = lex::grammar<test_spec, struct P, struct primary_prod>;
    struct primary_prod : lex::rule_production<primary_prod, grammar>
    {
        static constexpr auto rule()
        {
            return number{};
        }
    };
    struct P : lex::operator_production<P, grammar>
    {
        using multiplication = r::bin_op_single<star>;
        using addition       = r::bin_op_single<plus, multiplication>;

        using expression = r::expression<r::primary<primary_prod>, addition>;
    };

    struct visitor
    {
        constexpr lex::static_token<number> operator()(primary_prod,
                                                       lex::static_token<number> number) const
        {
            return number;
        }

        int           operator()(lex::callback_result_of<P>) const;
        constexpr int operator()(P, lex::static_token<number> num) const
        {
            return number::parse(num);
        }
        constexpr int operator()(P, int lhs, star, int rhs) const
        {
            return lhs * rhs;
        }
        constexpr int operator()(P, int lhs, plus, int rhs) const
        {
            return lhs + rhs;
        }

        constexpr void operator()(lex::unexpected_token<grammar, primary_prod, number>,
                                  const lex::tokenizer<test_spec>&) const
        {}
        constexpr void operator()(lex::unexpected_token<grammar, P, star>,
                                  const lex::tokenizer<test_spec>&) const
        {}
        constexpr void operator()(lex::unexpected_token<grammar, P, plus>,
                                  const lex::tokenizer<test_spec>&) const
        {}
    };

    constexpr auto r0 = parse<P>(visitor{}, "4");
    verify(r0, 4);

    constexpr auto r1 = parse<P>(visitor{}, "1 + 3");
    verify(r1, 4);

    constexpr auto r2 = parse<P>(visitor{}, "1 * 4");
    verify(r2, 4);

    constexpr auto r3 = parse<P>(visitor{}, "1 * 2 + 3");
    verify(r3, 5);

    constexpr auto r4 = parse<P>(visitor{}, "1 + 2 * 3");
    verify(r4, 7);

    constexpr auto r5 = parse<P>(visitor{}, "1 * 2 + 3 * 4");
    verify(r5, 14);

    constexpr auto r6 = parse<P>(visitor{}, "1 +");
    verify(r6, 0);

    constexpr auto r7 = parse<P>(visitor{}, "1 * 2 + ");
    verify(r7, 0);
}
