// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/lex/operator_production.hpp>

#include <catch.hpp>

#include <foonathan/lex/ascii.hpp>
#include <foonathan/lex/token_production.hpp>

namespace lex = foonathan::lex;

namespace
{
using test_spec
    = lex::token_spec<struct whitespace, struct number, struct plus, struct minus, struct star,
                      struct exclamation, struct paren_open, struct paren_close>;

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
struct exclamation : lex::literal_token<'!'>
{};
struct paren_open : lex::literal_token<'('>
{};
struct paren_close : lex::literal_token<')'>
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

void verify(lex::parse_result<int> result, unmatched_t)
{
    REQUIRE(result.is_unmatched());
}

void verify(lex::parse_result<int> result, int expected)
{
    REQUIRE(result.is_success());
    REQUIRE(result.value() == expected);
}
} // namespace

TEST_CASE("operator_production: pre_op_single")
{
    using grammar = lex::grammar<test_spec, struct P, struct primary>;
    struct primary : lex::token_production<primary, grammar, number>
    {};
    struct P : lex::operator_production<P, grammar>
    {
        static constexpr auto rule()
        {
            namespace r = lex::operator_rule;

            auto atom   = r::atom<primary>;
            auto negate = r::pre_op_single<minus>(atom);
            auto not_   = r::pre_op_single<exclamation>(negate);

            return not_;
        }
    };

    struct visitor
    {
        constexpr lex::static_token<number> operator()(primary,
                                                       lex::static_token<number> number) const
        {
            return number;
        }

        int           operator()(lex::callback_result_of<P>) const;
        constexpr int operator()(P, lex::static_token<number> num) const
        {
            return number::parse(num);
        }
        constexpr int operator()(P, minus, int value) const
        {
            return -value;
        }
        constexpr int operator()(P, exclamation, int value) const
        {
            return !value;
        }

        constexpr void operator()(lex::unexpected_token<grammar, primary, number>,
                                  const lex::tokenizer<test_spec>&) const
        {}
    };

    constexpr auto r0 = parse<P>(visitor{}, "4");
    verify(r0, 4);

    constexpr auto r1 = parse<P>(visitor{}, "-3");
    verify(r1, -3);

    constexpr auto r2 = parse<P>(visitor{}, "!0");
    verify(r2, 1);

    constexpr auto r3 = parse<P>(visitor{}, "!-2");
    verify(r3, 0);

    constexpr auto r4 = parse<P>(visitor{}, "--2");
    verify(r4, unmatched);

    constexpr auto r5 = parse<P>(visitor{}, "!!");
    verify(r5, unmatched);

    constexpr auto r6 = parse<P>(visitor{}, "-!2");
    verify(r6, unmatched);
}

TEST_CASE("operator_production: pre_op_chain")
{
    using grammar = lex::grammar<test_spec, struct P, struct primary>;
    struct primary : lex::token_production<primary, grammar, number>
    {};
    struct P : lex::operator_production<P, grammar>
    {
        static constexpr auto rule()
        {
            namespace r = lex::operator_rule;

            auto atom   = r::atom<primary>;
            auto negate = r::pre_op_chain<minus>(atom);
            auto not_   = r::pre_op_chain<exclamation>(negate);

            return not_;
        }
    };

    struct visitor
    {
        constexpr lex::static_token<number> operator()(primary,
                                                       lex::static_token<number> number) const
        {
            return number;
        }

        int           operator()(lex::callback_result_of<P>) const;
        constexpr int operator()(P, lex::static_token<number> num) const
        {
            return number::parse(num);
        }
        constexpr int operator()(P, minus, int value) const
        {
            return -value;
        }
        constexpr int operator()(P, exclamation, int value) const
        {
            return !value;
        }

        constexpr void operator()(lex::unexpected_token<grammar, primary, number>,
                                  const lex::tokenizer<test_spec>&) const
        {}
    };

    constexpr auto r0 = parse<P>(visitor{}, "4");
    verify(r0, 4);

    constexpr auto r1 = parse<P>(visitor{}, "-3");
    verify(r1, -3);

    constexpr auto r2 = parse<P>(visitor{}, "!0");
    verify(r2, 1);

    constexpr auto r3 = parse<P>(visitor{}, "!-2");
    verify(r3, 0);

    constexpr auto r4 = parse<P>(visitor{}, "--2");
    verify(r4, 2);

    constexpr auto r5 = parse<P>(visitor{}, "!!1");
    verify(r5, 1);

    constexpr auto r6 = parse<P>(visitor{}, "!!--1");
    verify(r6, 1);

    constexpr auto r7 = parse<P>(visitor{}, "-!2");
    verify(r7, unmatched);
}

TEST_CASE("operator_production: bin_op_single")
{
    using grammar = lex::grammar<test_spec, struct P, struct primary>;
    struct primary : lex::token_production<primary, grammar, number>
    {};
    struct P : lex::operator_production<P, grammar>
    {
        static constexpr auto rule()
        {
            namespace r = lex::operator_rule;

            auto atom           = r::atom<primary>;
            auto multiplication = r::bin_op_single<star>(atom);
            auto addition       = r::bin_op_single<plus, minus>(multiplication);

            return addition;
        }
    };

    struct visitor
    {
        constexpr lex::static_token<number> operator()(primary,
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
        constexpr int operator()(P, int lhs, minus, int rhs) const
        {
            return lhs - rhs;
        }

        constexpr void operator()(lex::unexpected_token<grammar, primary, number>,
                                  const lex::tokenizer<test_spec>&) const
        {}
    };

    constexpr auto r0 = parse<P>(visitor{}, "4");
    verify(r0, 4);

    {
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
    }
    {
        constexpr auto r1 = parse<P>(visitor{}, "1 - 3");
        verify(r1, -2);

        constexpr auto r2 = parse<P>(visitor{}, "1 * 4");
        verify(r2, 4);

        constexpr auto r3 = parse<P>(visitor{}, "1 * 2 - 3");
        verify(r3, -1);

        constexpr auto r4 = parse<P>(visitor{}, "1 - 2 * 3");
        verify(r4, -5);

        constexpr auto r5 = parse<P>(visitor{}, "1 * 2 - 3 * 4");
        verify(r5, -10);
    }

    constexpr auto r6 = parse<P>(visitor{}, "1 +");
    verify(r6, unmatched);

    constexpr auto r7 = parse<P>(visitor{}, "1 * 2 + ");
    verify(r7, unmatched);

    constexpr auto r8 = parse<P>(visitor{}, "1 + 2 + 3");
    verify(r8, unmatched);
}

TEST_CASE("operator_production: bin_op_single + pre_op_single")
{
    using grammar = lex::grammar<test_spec, struct P, struct primary>;
    struct primary : lex::token_production<primary, grammar, number>
    {};
    struct P : lex::operator_production<P, grammar>
    {
        static constexpr auto rule()
        {
            namespace r = lex::operator_rule;

            auto atom           = r::atom<primary>;
            auto negate         = r::pre_op_single<minus>(atom);
            auto multiplication = r::bin_op_single<star>(negate);
            auto addition       = r::bin_op_single<plus>(multiplication);

            return addition;
        }
    };

    struct visitor
    {
        constexpr lex::static_token<number> operator()(primary,
                                                       lex::static_token<number> number) const
        {
            return number;
        }

        int           operator()(lex::callback_result_of<P>) const;
        constexpr int operator()(P, lex::static_token<number> num) const
        {
            return number::parse(num);
        }
        constexpr int operator()(P, minus, int value) const
        {
            return -value;
        }
        constexpr int operator()(P, int lhs, star, int rhs) const
        {
            return lhs * rhs;
        }
        constexpr int operator()(P, int lhs, plus, int rhs) const
        {
            return lhs + rhs;
        }

        constexpr void operator()(lex::unexpected_token<grammar, primary, number>,
                                  const lex::tokenizer<test_spec>&) const
        {}
    };

    constexpr auto r0 = parse<P>(visitor{}, "4");
    verify(r0, 4);

    constexpr auto r1 = parse<P>(visitor{}, "-1 + 3");
    verify(r1, 2);

    constexpr auto r2 = parse<P>(visitor{}, "1 * -4");
    verify(r2, -4);

    constexpr auto r3 = parse<P>(visitor{}, "1 * -2 + 3");
    verify(r3, 1);

    constexpr auto r4 = parse<P>(visitor{}, "-1 + 2 * 3");
    verify(r4, 5);

    constexpr auto r5 = parse<P>(visitor{}, "-1 * -2 + 3 * 4");
    verify(r5, 14);

    constexpr auto r6 = parse<P>(visitor{}, "1 + -");
    verify(r6, unmatched);
}

TEST_CASE("operator_production: pre_op_single + bin_op_single")
{
    using grammar = lex::grammar<test_spec, struct P, struct primary>;
    struct primary : lex::token_production<primary, grammar, number>
    {};
    struct P : lex::operator_production<P, grammar>
    {
        static constexpr auto rule()
        {
            namespace r = lex::operator_rule;

            auto atom     = r::atom<primary>;
            auto addition = r::bin_op_single<plus>(atom);
            auto not_     = r::pre_op_single<exclamation>(addition);

            return not_;
        }
    };

    struct visitor
    {
        constexpr lex::static_token<number> operator()(primary,
                                                       lex::static_token<number> number) const
        {
            return number;
        }

        int           operator()(lex::callback_result_of<P>) const;
        constexpr int operator()(P, lex::static_token<number> num) const
        {
            return number::parse(num);
        }
        constexpr int operator()(P, exclamation, int value) const
        {
            return !value;
        }
        constexpr int operator()(P, int lhs, plus, int rhs) const
        {
            return lhs + rhs;
        }

        constexpr void operator()(lex::unexpected_token<grammar, primary, number>,
                                  const lex::tokenizer<test_spec>&) const
        {}
    };

    constexpr auto r0 = parse<P>(visitor{}, "4");
    verify(r0, 4);

    constexpr auto r1 = parse<P>(visitor{}, "1 + 3");
    verify(r1, 4);

    constexpr auto r2 = parse<P>(visitor{}, "!1");
    verify(r2, 0);

    constexpr auto r3 = parse<P>(visitor{}, "! 1+2");
    verify(r3, 0);

    constexpr auto r4 = parse<P>(visitor{}, "! 0 + 0");
    verify(r4, 1);

    constexpr auto r5 = parse<P>(visitor{}, "! 1 + ");
    verify(r5, unmatched);
}

TEST_CASE("operator_production: bin_op_left")
{
    using grammar = lex::grammar<test_spec, struct P, struct primary>;
    struct primary : lex::token_production<primary, grammar, number>
    {};
    struct P : lex::operator_production<P, grammar>
    {
        static constexpr auto rule()
        {
            namespace r = lex::operator_rule;

            auto atom     = r::atom<primary>;
            auto addition = r::bin_op_left<minus>(atom);

            return addition;
        }
    };

    struct visitor
    {
        constexpr lex::static_token<number> operator()(primary,
                                                       lex::static_token<number> number) const
        {
            return number;
        }

        int           operator()(lex::callback_result_of<P>) const;
        constexpr int operator()(P, lex::static_token<number> num) const
        {
            return number::parse(num);
        }
        constexpr int operator()(P, int lhs, minus, int rhs) const
        {
            return lhs - rhs;
        }

        constexpr void operator()(lex::unexpected_token<grammar, primary, number>,
                                  const lex::tokenizer<test_spec>&) const
        {}
    };

    constexpr auto r0 = parse<P>(visitor{}, "4");
    verify(r0, 4);

    constexpr auto r1 = parse<P>(visitor{}, "1 - 2");
    verify(r1, -1);

    constexpr auto r2 = parse<P>(visitor{}, "1 - 2 - 3");
    verify(r2, -4);

    constexpr auto r3 = parse<P>(visitor{}, "1 - 2 - ");
    verify(r3, unmatched);
}

TEST_CASE("operator_production: bin_op_right")
{
    using grammar = lex::grammar<test_spec, struct P, struct primary>;
    struct primary : lex::token_production<primary, grammar, number>
    {};
    struct P : lex::operator_production<P, grammar>
    {
        static constexpr auto rule()
        {
            namespace r = lex::operator_rule;

            auto atom     = r::atom<primary>;
            auto addition = r::bin_op_right<minus>(atom);

            return addition;
        }
    };

    struct visitor
    {
        constexpr lex::static_token<number> operator()(primary,
                                                       lex::static_token<number> number) const
        {
            return number;
        }

        int           operator()(lex::callback_result_of<P>) const;
        constexpr int operator()(P, lex::static_token<number> num) const
        {
            return number::parse(num);
        }
        constexpr int operator()(P, int lhs, minus, int rhs) const
        {
            return lhs - rhs;
        }

        constexpr void operator()(lex::unexpected_token<grammar, primary, number>,
                                  const lex::tokenizer<test_spec>&) const
        {}
    };

    constexpr auto r0 = parse<P>(visitor{}, "4");
    verify(r0, 4);

    constexpr auto r1 = parse<P>(visitor{}, "1 - 2");
    verify(r1, -1);

    constexpr auto r2 = parse<P>(visitor{}, "1 - 2 - 3");
    verify(r2, 2);

    constexpr auto r3 = parse<P>(visitor{}, "1 - 2 - ");
    verify(r3, unmatched);
}

TEST_CASE("operator_production: parenthesized")
{
    using grammar = lex::grammar<test_spec, struct P, struct primary>;
    struct primary : lex::token_production<primary, grammar, number>
    {};
    struct P : lex::operator_production<P, grammar>
    {
        static constexpr auto rule()
        {
            namespace r = lex::operator_rule;

            auto atom           = r::atom<primary> / r::parenthesized<paren_open, paren_close>;
            auto multiplication = r::bin_op_single<star>(atom);
            auto addition       = r::bin_op_single<plus>(multiplication);

            return addition;
        }
    };

    struct visitor
    {
        constexpr lex::static_token<number> operator()(primary,
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

        constexpr void operator()(lex::unexpected_token<grammar, primary, number>,
                                  const lex::tokenizer<test_spec>&) const
        {}
        constexpr void operator()(lex::unexpected_token<grammar, P, paren_close>,
                                  const lex::tokenizer<test_spec>&) const
        {}
    };

    constexpr auto r0 = parse<P>(visitor{}, "4");
    verify(r0, 4);

    auto r1 = parse<P>(visitor{}, "(1 + 3)");
    verify(r1, 4);

    constexpr auto r2 = parse<P>(visitor{}, "(1 * ((4)))");
    verify(r2, 4);

    constexpr auto r3 = parse<P>(visitor{}, "2 * (2 + 3)");
    verify(r3, 10);

    constexpr auto r4 = parse<P>(visitor{}, "(1 + 2) * 3");
    verify(r4, 9);

    constexpr auto r5 = parse<P>(visitor{}, "(1 * (2 + 3)) * 4");
    verify(r5, 20);

    constexpr auto r6 = parse<P>(visitor{}, "1 + (");
    verify(r6, unmatched);

    constexpr auto r7 = parse<P>(visitor{}, "1 * (2 + ");
    verify(r7, unmatched);
}
