// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/lex/operator_production.hpp>

#include <doctest.h>

#include <foonathan/lex/ascii.hpp>
#include <foonathan/lex/rule_production.hpp>

#include "test.hpp"

namespace lex = foonathan::lex;

namespace
{
using test_spec = lex::token_spec<struct whitespace, struct number, struct plus, struct minus,
                                  struct star, struct exclamation, struct tilde, struct ampersand,
                                  struct equal, struct paren_open, struct paren_close>;

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
struct tilde : lex::literal_token<'~'>
{};
struct ampersand : lex::literal_token<'&'>
{};
struct equal : lex::literal_token<'='>
{};
struct paren_open : lex::literal_token<'('>
{};
struct paren_close : lex::literal_token<')'>
{};

template <class TLP, typename Func, std::size_t N>
constexpr auto parse(Func&& f, const char (&str)[N])
{
    lex::tokenizer<test_spec> tokenizer(str);
    auto                      result = TLP::parse(tokenizer, f);
    if (!tokenizer.is_done())
        return decltype(result)();
    else
        return result;
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

TEST_CASE("operator_production: pre_op_single")
{
    using grammar = lex::grammar<test_spec, struct P>;
    struct P : lex::operator_production<P, grammar>
    {
        static constexpr auto rule()
        {
            namespace r = lex::operator_rule;

            auto atom   = r::atom<number>;
            auto negate = r::pre_op_single<minus>(atom);
            auto not_   = r::pre_op_single<exclamation>(negate);

            return not_;
        }
    };

    struct visitor
    {
        int result_of(P) const;

        constexpr int production(P, lex::static_token<number> num) const
        {
            return number::parse(num);
        }

        constexpr int production(P, minus, int value) const
        {
            return -value;
        }

        constexpr int production(P, exclamation, int value) const
        {
            return !value;
        }

        constexpr void error(lex::unexpected_token<grammar, P, number>,
                             const lex::tokenizer<test_spec>&) const
        {}
    };

    FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "4");
    verify(r0, 4);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r1 = parse<P>(visitor{}, "-3");
    verify(r1, -3);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "!0");
    verify(r2, 1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "!-2");
    verify(r3, 0);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r4 = parse<P>(visitor{}, "--2");
    verify(r4, unmatched);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r5 = parse<P>(visitor{}, "!!");
    verify(r5, unmatched);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r6 = parse<P>(visitor{}, "-!2");
    verify(r6, unmatched);
}

TEST_CASE("operator_production: pre_op_chain")
{
    using grammar = lex::grammar<test_spec, struct P>;
    struct P : lex::operator_production<P, grammar>
    {
        static constexpr auto rule()
        {
            namespace r = lex::operator_rule;

            auto atom   = r::atom<number>;
            auto negate = r::pre_op_chain<minus>(atom);
            auto not_   = r::pre_op_chain<exclamation>(negate);

            return not_;
        }
    };

    struct visitor
    {
        int result_of(P) const;

        constexpr int production(P, lex::static_token<number> num) const
        {
            return number::parse(num);
        }

        constexpr int production(P, minus, int value) const
        {
            return -value;
        }

        constexpr int production(P, exclamation, int value) const
        {
            return !value;
        }

        constexpr void error(lex::unexpected_token<grammar, P, number>,
                             const lex::tokenizer<test_spec>&) const
        {}
    };

    FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "4");
    verify(r0, 4);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r1 = parse<P>(visitor{}, "-3");
    verify(r1, -3);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "!0");
    verify(r2, 1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "!-2");
    verify(r3, 0);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r4 = parse<P>(visitor{}, "--2");
    verify(r4, 2);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r5 = parse<P>(visitor{}, "!!1");
    verify(r5, 1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r6 = parse<P>(visitor{}, "!!--1");
    verify(r6, 1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r7 = parse<P>(visitor{}, "-!2");
    verify(r7, unmatched);
}

TEST_CASE("operator_production: post_op_single")
{
    using grammar = lex::grammar<test_spec, struct P>;
    struct P : lex::operator_production<P, grammar>
    {
        static constexpr auto rule()
        {
            namespace r = lex::operator_rule;

            auto atom   = r::atom<number>;
            auto negate = r::post_op_single<minus>(atom);
            auto not_   = r::post_op_single<exclamation>(negate);

            return not_;
        }
    };

    struct visitor
    {
        int result_of(P) const;

        constexpr int production(P, lex::static_token<number> num) const
        {
            return number::parse(num);
        }

        constexpr int production(P, int value, minus) const
        {
            return -value;
        }

        constexpr int production(P, int value, exclamation) const
        {
            return !value;
        }

        constexpr void error(lex::unexpected_token<grammar, P, number>,
                             const lex::tokenizer<test_spec>&) const
        {}
    };

    FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "4");
    verify(r0, 4);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r1 = parse<P>(visitor{}, "3-");
    verify(r1, -3);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "0!");
    verify(r2, 1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "2-!");
    verify(r3, 0);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r4 = parse<P>(visitor{}, "2--");
    verify(r4, unmatched);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r5 = parse<P>(visitor{}, "!!");
    verify(r5, unmatched);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r6 = parse<P>(visitor{}, "2-!-");
    verify(r6, unmatched);
}

TEST_CASE("operator_production: post_op_chain")
{
    using grammar = lex::grammar<test_spec, struct P>;
    struct P : lex::operator_production<P, grammar>
    {
        static constexpr auto rule()
        {
            namespace r = lex::operator_rule;

            auto atom   = r::atom<number>;
            auto negate = r::post_op_chain<minus>(atom);
            auto not_   = r::post_op_chain<exclamation>(negate);

            return not_;
        }
    };

    struct visitor
    {
        int result_of(P) const;

        constexpr int production(P, lex::static_token<number> num) const
        {
            return number::parse(num);
        }

        constexpr int production(P, int value, minus) const
        {
            return -value;
        }

        constexpr int production(P, int value, exclamation) const
        {
            return !value;
        }

        constexpr void error(lex::unexpected_token<grammar, P, number>,
                             const lex::tokenizer<test_spec>&) const
        {}
    };

    FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "4");
    verify(r0, 4);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r1 = parse<P>(visitor{}, "3-");
    verify(r1, -3);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "0!");
    verify(r2, 1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "2-!");
    verify(r3, 0);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r4 = parse<P>(visitor{}, "2--");
    verify(r4, 2);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r5 = parse<P>(visitor{}, "1!!");
    verify(r5, 1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r6 = parse<P>(visitor{}, "1--!!");
    verify(r6, 1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r7 = parse<P>(visitor{}, "2!-");
    verify(r7, unmatched);
}

TEST_CASE("operator_production: bin_op_single")
{
    using grammar = lex::grammar<test_spec, struct P>;
    struct P : lex::operator_production<P, grammar>
    {
        static constexpr auto rule()
        {
            namespace r = lex::operator_rule;

            auto atom           = r::atom<number>;
            auto multiplication = r::bin_op_single<star>(atom);
            auto addition       = r::bin_op_single<plus, minus>(multiplication);

            return addition;
        }
    };

    struct visitor
    {
        int result_of(P) const;

        constexpr int production(P, lex::static_token<number> num) const
        {
            return number::parse(num);
        }

        constexpr int production(P, int lhs, star, int rhs) const
        {
            return lhs * rhs;
        }

        constexpr int production(P, int lhs, plus, int rhs) const
        {
            return lhs + rhs;
        }

        constexpr int production(P, int lhs, minus, int rhs) const
        {
            return lhs - rhs;
        }

        constexpr void error(lex::unexpected_token<grammar, P, number>,
                             const lex::tokenizer<test_spec>&) const
        {}
    };

    FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "4");
    verify(r0, 4);

    {
        FOONATHAN_LEX_TEST_CONSTEXPR auto r1 = parse<P>(visitor{}, "1 + 3");
        verify(r1, 4);

        FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "1 * 4");
        verify(r2, 4);

        FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "1 * 2 + 3");
        verify(r3, 5);

        FOONATHAN_LEX_TEST_CONSTEXPR auto r4 = parse<P>(visitor{}, "1 + 2 * 3");
        verify(r4, 7);

        FOONATHAN_LEX_TEST_CONSTEXPR auto r5 = parse<P>(visitor{}, "1 * 2 + 3 * 4");
        verify(r5, 14);
    }
    {
        FOONATHAN_LEX_TEST_CONSTEXPR auto r1 = parse<P>(visitor{}, "1 - 3");
        verify(r1, -2);

        FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "1 * 4");
        verify(r2, 4);

        FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "1 * 2 - 3");
        verify(r3, -1);

        FOONATHAN_LEX_TEST_CONSTEXPR auto r4 = parse<P>(visitor{}, "1 - 2 * 3");
        verify(r4, -5);

        FOONATHAN_LEX_TEST_CONSTEXPR auto r5 = parse<P>(visitor{}, "1 * 2 - 3 * 4");
        verify(r5, -10);
    }

    FOONATHAN_LEX_TEST_CONSTEXPR auto r6 = parse<P>(visitor{}, "1 +");
    verify(r6, unmatched);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r7 = parse<P>(visitor{}, "1 * 2 + ");
    verify(r7, unmatched);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r8 = parse<P>(visitor{}, "1 + 2 + 3");
    verify(r8, unmatched);
}

TEST_CASE("operator_production: bin_op_single + pre_op_single")
{
    using grammar = lex::grammar<test_spec, struct P>;
    struct P : lex::operator_production<P, grammar>
    {
        static constexpr auto rule()
        {
            namespace r = lex::operator_rule;

            auto atom           = r::atom<number>;
            auto negate         = r::pre_op_single<minus>(atom);
            auto multiplication = r::bin_op_single<star>(negate);
            auto addition       = r::bin_op_single<plus>(multiplication);

            return addition;
        }
    };

    struct visitor
    {
        int result_of(P) const;

        constexpr int production(P, lex::static_token<number> num) const
        {
            return number::parse(num);
        }

        constexpr int production(P, minus, int value) const
        {
            return -value;
        }

        constexpr int production(P, int lhs, star, int rhs) const
        {
            return lhs * rhs;
        }

        constexpr int production(P, int lhs, plus, int rhs) const
        {
            return lhs + rhs;
        }

        constexpr void error(lex::unexpected_token<grammar, P, number>,
                             const lex::tokenizer<test_spec>&) const
        {}
    };

    FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "4");
    verify(r0, 4);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r1 = parse<P>(visitor{}, "-1 + 3");
    verify(r1, 2);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "1 * -4");
    verify(r2, -4);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "1 * -2 + 3");
    verify(r3, 1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r4 = parse<P>(visitor{}, "-1 + 2 * 3");
    verify(r4, 5);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r5 = parse<P>(visitor{}, "-1 * -2 + 3 * 4");
    verify(r5, 14);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r6 = parse<P>(visitor{}, "1 + -");
    verify(r6, unmatched);
}

TEST_CASE("operator_production: pre_op_single + bin_op_single")
{
    using grammar = lex::grammar<test_spec, struct P>;
    struct P : lex::operator_production<P, grammar>
    {
        static constexpr auto rule()
        {
            namespace r = lex::operator_rule;

            auto atom     = r::atom<number>;
            auto addition = r::bin_op_single<plus>(atom);
            auto not_     = r::pre_op_single<exclamation>(addition);

            return not_;
        }
    };

    struct visitor
    {
        int result_of(P) const;

        constexpr int production(P, lex::static_token<number> num) const
        {
            return number::parse(num);
        }

        constexpr int production(P, exclamation, int value) const
        {
            return !value;
        }

        constexpr int production(P, int lhs, plus, int rhs) const
        {
            return lhs + rhs;
        }

        constexpr void error(lex::unexpected_token<grammar, P, number>,
                             const lex::tokenizer<test_spec>&) const
        {}
    };

    FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "4");
    verify(r0, 4);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r1 = parse<P>(visitor{}, "1 + 3");
    verify(r1, 4);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "!1");
    verify(r2, 0);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "! 1+2");
    verify(r3, 0);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r4 = parse<P>(visitor{}, "! 0 + 0");
    verify(r4, 1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r5 = parse<P>(visitor{}, "! 1 + ");
    verify(r5, unmatched);
}

TEST_CASE("operator_production: bin_op_single + post_op_single")
{
    using grammar = lex::grammar<test_spec, struct P>;
    struct P : lex::operator_production<P, grammar>
    {
        static constexpr auto rule()
        {
            namespace r = lex::operator_rule;

            auto atom           = r::atom<number>;
            auto negate         = r::post_op_single<minus>(atom);
            auto multiplication = r::bin_op_single<star>(negate);
            auto addition       = r::bin_op_single<plus>(multiplication);

            return addition;
        }
    };

    struct visitor
    {
        int result_of(P) const;

        constexpr int production(P, lex::static_token<number> num) const
        {
            return number::parse(num);
        }

        constexpr int production(P, int value, minus) const
        {
            return -value;
        }

        constexpr int production(P, int lhs, star, int rhs) const
        {
            return lhs * rhs;
        }

        constexpr int production(P, int lhs, plus, int rhs) const
        {
            return lhs + rhs;
        }

        constexpr void error(lex::unexpected_token<grammar, P, number>,
                             const lex::tokenizer<test_spec>&) const
        {}
    };

    FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "4");
    verify(r0, 4);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r1 = parse<P>(visitor{}, "1- + 3");
    verify(r1, 2);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "1 * 4-");
    verify(r2, -4);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "1 * 2- + 3");
    verify(r3, 1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r4 = parse<P>(visitor{}, "1- + 2 * 3");
    verify(r4, 5);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r5 = parse<P>(visitor{}, "1- * 2- + 3 * 4");
    verify(r5, 14);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r6 = parse<P>(visitor{}, "1 + -");
    verify(r6, unmatched);
}

TEST_CASE("operator_production: post_op_single + bin_op_single")
{
    using grammar = lex::grammar<test_spec, struct P>;
    struct P : lex::operator_production<P, grammar>
    {
        static constexpr auto rule()
        {
            namespace r = lex::operator_rule;

            auto atom     = r::atom<number>;
            auto addition = r::bin_op_single<plus>(atom);
            auto not_     = r::post_op_single<exclamation>(addition);

            return not_;
        }
    };

    struct visitor
    {
        int result_of(P) const;

        constexpr int production(P, lex::static_token<number> num) const
        {
            return number::parse(num);
        }

        constexpr int production(P, int value, exclamation) const
        {
            return !value;
        }

        constexpr int production(P, int lhs, plus, int rhs) const
        {
            return lhs + rhs;
        }

        constexpr void error(lex::unexpected_token<grammar, P, number>,
                             const lex::tokenizer<test_spec>&) const
        {}
    };

    FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "4");
    verify(r0, 4);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r1 = parse<P>(visitor{}, "1 + 3");
    verify(r1, 4);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "1!");
    verify(r2, 0);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "1+2 !");
    verify(r3, 0);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r4 = parse<P>(visitor{}, "0 + 0 !");
    verify(r4, 1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r5 = parse<P>(visitor{}, "1 + !");
    verify(r5, unmatched);
}

TEST_CASE("operator_production: bin_op_left")
{
    using grammar = lex::grammar<test_spec, struct P>;
    struct P : lex::operator_production<P, grammar>
    {
        static constexpr auto rule()
        {
            namespace r = lex::operator_rule;

            auto atom           = r::atom<number>;
            auto multiplication = r::bin_op_single<star>(atom);
            auto addition       = r::bin_op_left<minus>(multiplication);

            return addition;
        }
    };

    struct visitor
    {
        int result_of(P) const;

        constexpr int production(P, lex::static_token<number> num) const
        {
            return number::parse(num);
        }

        constexpr int production(P, int lhs, star, int rhs) const
        {
            return lhs * rhs;
        }

        constexpr int production(P, int lhs, minus, int rhs) const
        {
            return lhs - rhs;
        }

        constexpr void error(lex::unexpected_token<grammar, P, number>,
                             const lex::tokenizer<test_spec>&) const
        {}
    };

    FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "4");
    verify(r0, 4);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r1 = parse<P>(visitor{}, "1 - 2");
    verify(r1, -1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "1 - 2 - 3");
    verify(r2, -4);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "1 * 2 - 2 - 3");
    verify(r3, -3);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r4 = parse<P>(visitor{}, "1 - 2 - 2 * 3");
    verify(r4, -7);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r5 = parse<P>(visitor{}, "1 - 2 - ");
    verify(r5, unmatched);
}

TEST_CASE("operator_production: bin_op_right")
{
    using grammar = lex::grammar<test_spec, struct P>;
    struct P : lex::operator_production<P, grammar>
    {
        static constexpr auto rule()
        {
            namespace r = lex::operator_rule;

            auto atom           = r::atom<number>;
            auto multiplication = r::bin_op_single<star>(atom);
            auto addition       = r::bin_op_right<minus>(multiplication);

            return addition;
        }
    };

    struct visitor
    {
        int result_of(P) const;

        constexpr int production(P, lex::static_token<number> num) const
        {
            return number::parse(num);
        }

        constexpr int production(P, int lhs, star, int rhs) const
        {
            return lhs * rhs;
        }

        constexpr int production(P, int lhs, minus, int rhs) const
        {
            return lhs - rhs;
        }

        constexpr void error(lex::unexpected_token<grammar, P, number>,
                             const lex::tokenizer<test_spec>&) const
        {}
    };

    FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "4");
    verify(r0, 4);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r1 = parse<P>(visitor{}, "1 - 2");
    verify(r1, -1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "1 - 2 - 3");
    verify(r2, 2);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "1 * 2 - 2 - 3");
    verify(r3, 3);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r4 = parse<P>(visitor{}, "1 - 2 - 2 * 3");
    verify(r4, 5);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r5 = parse<P>(visitor{}, "1 - 2 - ");
    verify(r5, unmatched);
}

TEST_CASE("operator_production: parenthesized")
{
    using grammar = lex::grammar<test_spec, struct P>;
    struct P : lex::operator_production<P, grammar>
    {
        static constexpr auto rule()
        {
            namespace r = lex::operator_rule;

            auto atom           = r::atom<number> / r::parenthesized<paren_open, paren_close>;
            auto multiplication = r::bin_op_single<star>(atom);
            auto addition       = r::bin_op_single<plus>(multiplication);

            return addition;
        }
    };

    struct visitor
    {
        int result_of(P) const;

        constexpr int production(P, lex::static_token<number> num) const
        {
            return number::parse(num);
        }

        constexpr int production(P, int lhs, star, int rhs) const
        {
            return lhs * rhs;
        }

        constexpr int production(P, int lhs, plus, int rhs) const
        {
            return lhs + rhs;
        }

        constexpr void error(lex::unexpected_token<grammar, P, number>,
                             const lex::tokenizer<test_spec>&) const
        {}

        constexpr void error(lex::unexpected_token<grammar, P, paren_close>,
                             const lex::tokenizer<test_spec>&) const
        {}
    };

    FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "4");
    verify(r0, 4);

    auto r1 = parse<P>(visitor{}, "(1 + 3)");
    verify(r1, 4);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "(1 * ((4)))");
    verify(r2, 4);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "2 * (2 + 3)");
    verify(r3, 10);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r4 = parse<P>(visitor{}, "(1 + 2) * 3");
    verify(r4, 9);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r5 = parse<P>(visitor{}, "(1 * (2 + 3)) * 4");
    verify(r5, 20);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r6 = parse<P>(visitor{}, "1 + (");
    verify(r6, unmatched);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r7 = parse<P>(visitor{}, "1 * (2 + ");
    verify(r7, unmatched);
}

TEST_CASE("operator_production: choice with single atom")
{
    using grammar = lex::grammar<test_spec, struct P>;
    struct P : lex::operator_production<P, grammar>
    {
        static constexpr auto rule()
        {
            namespace r = lex::operator_rule;

            auto atom = r::atom<number>;

            auto multiplication = r::bin_op_left<star>(atom);
            auto addition       = r::bin_op_left<plus>(atom);
            auto difference     = r::bin_op_left<minus>(atom);

            return multiplication / addition / difference;
        }
    };

    struct visitor
    {
        int result_of(P) const;

        constexpr int production(P, lex::static_token<number> num) const
        {
            return number::parse(num);
        }

        constexpr int production(P, int lhs, star, int rhs) const
        {
            return lhs * rhs;
        }

        constexpr int production(P, int lhs, plus, int rhs) const
        {
            return lhs + rhs;
        }

        constexpr int production(P, int lhs, minus, int rhs) const
        {
            return lhs - rhs;
        }

        constexpr void error(lex::unexpected_token<grammar, P, number>,
                             const lex::tokenizer<test_spec>&) const
        {}
    };

    FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "1");
    verify(r0, 1);

    auto r1 = parse<P>(visitor{}, "1 + 2 + 3");
    verify(r1, 6);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "2 * 2 * 3");
    verify(r2, 12);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "1 - 2 - 3");
    verify(r3, -4);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r4 = parse<P>(visitor{}, "1 + 2 - 3");
    verify(r4, unmatched);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r5 = parse<P>(visitor{}, "1 * 2 - 3");
    verify(r5, unmatched);
}

TEST_CASE("operator_production: choice with unary")
{
    using grammar = lex::grammar<test_spec, struct P>;
    struct P : lex::operator_production<P, grammar>
    {
        static constexpr auto rule()
        {
            namespace r = lex::operator_rule;

            auto atom = r::atom<number>;

            auto multiplication = r::bin_op_left<star>(atom);

            auto negate   = r::pre_op_single<minus>(atom);
            auto addition = r::bin_op_left<plus>(negate);

            return multiplication / addition;
        }
    };

    struct visitor
    {
        int result_of(P) const;

        constexpr int production(P, lex::static_token<number> num) const
        {
            return number::parse(num);
        }

        constexpr int production(P, int lhs, star, int rhs) const
        {
            return lhs * rhs;
        }

        constexpr int production(P, int lhs, plus, int rhs) const
        {
            return lhs + rhs;
        }

        constexpr int production(P, minus, int rhs) const
        {
            return -rhs;
        }

        constexpr void error(lex::unexpected_token<grammar, P, number>,
                             const lex::tokenizer<test_spec>&) const
        {}
    };

    FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "1");
    verify(r0, 1);

    auto r1 = parse<P>(visitor{}, "1 + 2 + 3");
    verify(r1, 6);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "2 * 2 * 3");
    verify(r2, 12);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "-4");
    verify(r3, -4);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r4 = parse<P>(visitor{}, "1 + 2 * 3");
    verify(r4, unmatched);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r5 = parse<P>(visitor{}, "-1 * 3");
    verify(r5, unmatched);
}

TEST_CASE("operator_production: production as operator")
{
    SUBCASE("pre_prod_single")
    {
        using grammar = lex::grammar<test_spec, struct P, struct op>;
        struct op : lex::rule_production<op, grammar>
        {
            static constexpr auto rule()
            {
                return minus{};
            }
        };

        struct P : lex::operator_production<P, grammar>
        {
            static FOONATHAN_LEX_TEST_CONSTEXPR auto rule()
            {
                namespace r = lex::operator_rule;

                auto atom = r::atom<number>;
                return r::pre_prod_single<minus>(op{}, atom);
            }
        };

        struct visitor
        {
            constexpr void production(op, minus) const {}

            int result_of(P) const;

            constexpr int production(P, lex::static_token<number> num) const
            {
                return number::parse(num);
            }

            constexpr int production(P, op, int rhs) const
            {
                return -rhs;
            }

            constexpr void error(lex::unexpected_token<grammar, P, number>,
                                 const lex::tokenizer<test_spec>&) const
            {}

            constexpr void error(lex::unexpected_token<grammar, op, minus>,
                                 const lex::tokenizer<test_spec>&) const
            {}
        };

        FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "1");
        verify(r0, 1);

        auto r1 = parse<P>(visitor{}, "-1");
        verify(r1, -1);

        FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "--1");
        verify(r2, unmatched);

        FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "-");
        verify(r3, unmatched);
    }
    SUBCASE("pre_prod_chain")
    {
        using grammar = lex::grammar<test_spec, struct P, struct op>;
        struct op : lex::rule_production<op, grammar>
        {
            static constexpr auto rule()
            {
                return minus{};
            }
        };

        struct P : lex::operator_production<P, grammar>
        {
            static constexpr auto rule()
            {
                namespace r = lex::operator_rule;

                auto atom = r::atom<number>;
                return r::pre_prod_chain<minus>(op{}, atom);
            }
        };

        struct visitor
        {
            constexpr void production(op, minus) const {}

            int result_of(P) const;

            constexpr int production(P, lex::static_token<number> num) const
            {
                return number::parse(num);
            }

            constexpr int production(P, op, int rhs) const
            {
                return -rhs;
            }

            constexpr void error(lex::unexpected_token<grammar, P, number>,
                                 const lex::tokenizer<test_spec>&) const
            {}

            constexpr void error(lex::unexpected_token<grammar, op, minus>,
                                 const lex::tokenizer<test_spec>&) const
            {}
        };

        FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "1");
        verify(r0, 1);

        auto r1 = parse<P>(visitor{}, "-1");
        verify(r1, -1);

        FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "--1");
        verify(r2, 1);

        FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "-");
        verify(r3, unmatched);
    }
    SUBCASE("post_prod_single")
    {
        using grammar = lex::grammar<test_spec, struct P, struct op>;
        struct op : lex::rule_production<op, grammar>
        {
            static constexpr auto rule()
            {
                return minus{};
            }
        };

        struct P : lex::operator_production<P, grammar>
        {
            static constexpr auto rule()
            {
                namespace r = lex::operator_rule;

                auto atom = r::atom<number>;
                return r::post_prod_single<minus>(op{}, atom);
            }
        };

        struct visitor
        {
            constexpr void production(op, minus) const {}

            int result_of(P) const;

            constexpr int production(P, lex::static_token<number> num) const
            {
                return number::parse(num);
            }

            constexpr int production(P, int lhs, op) const
            {
                return -lhs;
            }

            constexpr void error(lex::unexpected_token<grammar, P, number>,
                                 const lex::tokenizer<test_spec>&) const
            {}

            constexpr void error(lex::unexpected_token<grammar, op, minus>,
                                 const lex::tokenizer<test_spec>&) const
            {}
        };

        FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "1");
        verify(r0, 1);

        auto r1 = parse<P>(visitor{}, "1-");
        verify(r1, -1);

        FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "1--");
        verify(r2, unmatched);

        FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "-");
        verify(r3, unmatched);
    }
    SUBCASE("post_prod_chain")
    {
        using grammar = lex::grammar<test_spec, struct P, struct op>;
        struct op : lex::rule_production<op, grammar>
        {
            static constexpr auto rule()
            {
                return minus{};
            }
        };

        struct P : lex::operator_production<P, grammar>
        {
            static constexpr auto rule()
            {
                namespace r = lex::operator_rule;

                auto atom = r::atom<number>;
                return r::post_prod_chain<minus>(op{}, atom);
            }
        };

        struct visitor
        {
            constexpr void production(op, minus) const {}

            int result_of(P) const;

            constexpr int production(P, lex::static_token<number> num) const
            {
                return number::parse(num);
            }

            constexpr int production(P, int lhs, op) const
            {
                return -lhs;
            }

            constexpr void error(lex::unexpected_token<grammar, P, number>,
                                 const lex::tokenizer<test_spec>&) const
            {}

            constexpr void error(lex::unexpected_token<grammar, op, minus>,
                                 const lex::tokenizer<test_spec>&) const
            {}
        };

        FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "1");
        verify(r0, 1);

        auto r1 = parse<P>(visitor{}, "1-");
        verify(r1, -1);

        FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "1--");
        verify(r2, 1);

        FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "-");
        verify(r3, unmatched);
    }
    SUBCASE("bin_prod_single")
    {
        using grammar = lex::grammar<test_spec, struct P, struct op>;
        struct op : lex::rule_production<op, grammar>
        {
            static constexpr auto rule()
            {
                return minus{};
            }
        };

        struct P : lex::operator_production<P, grammar>
        {
            static constexpr auto rule()
            {
                namespace r = lex::operator_rule;

                auto atom = r::atom<number>;
                return r::bin_prod_single<minus>(op{}, atom);
            }
        };

        struct visitor
        {
            constexpr void production(op, minus) const {}

            int result_of(P) const;

            constexpr int production(P, lex::static_token<number> num) const
            {
                return number::parse(num);
            }

            constexpr int production(P, int lhs, op, int rhs) const
            {
                return lhs - rhs;
            }

            constexpr void error(lex::unexpected_token<grammar, P, number>,
                                 const lex::tokenizer<test_spec>&) const
            {}

            constexpr void error(lex::unexpected_token<grammar, op, minus>,
                                 const lex::tokenizer<test_spec>&) const
            {}
        };

        FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "1");
        verify(r0, 1);

        auto r1 = parse<P>(visitor{}, "1 - 2");
        verify(r1, -1);

        FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "1 - 2 - 3");
        verify(r2, unmatched);

        FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "1 -");
        verify(r3, unmatched);
    }
    SUBCASE("bin_prod_left")
    {
        using grammar = lex::grammar<test_spec, struct P, struct op>;
        struct op : lex::rule_production<op, grammar>
        {
            static constexpr auto rule()
            {
                return minus{};
            }
        };

        struct P : lex::operator_production<P, grammar>
        {
            static constexpr auto rule()
            {
                namespace r = lex::operator_rule;

                auto atom = r::atom<number>;
                return r::bin_prod_left<minus>(op{}, atom);
            }
        };

        struct visitor
        {
            constexpr void production(op, minus) const {}

            int result_of(P) const;

            constexpr int production(P, lex::static_token<number> num) const
            {
                return number::parse(num);
            }

            constexpr int production(P, int lhs, op, int rhs) const
            {
                return lhs - rhs;
            }

            constexpr void error(lex::unexpected_token<grammar, P, number>,
                                 const lex::tokenizer<test_spec>&) const
            {}

            constexpr void error(lex::unexpected_token<grammar, op, minus>,
                                 const lex::tokenizer<test_spec>&) const
            {}
        };

        FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "1");
        verify(r0, 1);

        auto r1 = parse<P>(visitor{}, "1 - 2");
        verify(r1, -1);

        FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "1 - 2 - 3");
        verify(r2, -4);

        FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "1 -");
        verify(r3, unmatched);
    }
    SUBCASE("bin_prod_right")
    {
        using grammar = lex::grammar<test_spec, struct P, struct op>;
        struct op : lex::rule_production<op, grammar>
        {
            static constexpr auto rule()
            {
                return minus{};
            }
        };

        struct P : lex::operator_production<P, grammar>
        {
            static constexpr auto rule()
            {
                namespace r = lex::operator_rule;

                auto atom = r::atom<number>;
                return r::bin_prod_right<minus>(op{}, atom);
            }
        };

        struct visitor
        {
            constexpr void production(op, minus) const {}

            int result_of(P) const;

            constexpr int production(P, lex::static_token<number> num) const
            {
                return number::parse(num);
            }

            constexpr int production(P, int lhs, op, int rhs) const
            {
                return lhs - rhs;
            }

            constexpr void error(lex::unexpected_token<grammar, P, number>,
                                 const lex::tokenizer<test_spec>&) const
            {}

            constexpr void error(lex::unexpected_token<grammar, op, minus>,
                                 const lex::tokenizer<test_spec>&) const
            {}
        };

        FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "1");
        verify(r0, 1);

        auto r1 = parse<P>(visitor{}, "1 - 2");
        verify(r1, -1);

        FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "1 - 2 - 3");
        verify(r2, 2);

        FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "1 -");
        verify(r3, unmatched);
    }
}

TEST_CASE("operator_production: expr")
{
    using grammar = lex::grammar<test_spec, struct P>;
    struct P : lex::operator_production<P, grammar>
    {
        static constexpr auto rule()
        {
            namespace r = lex::operator_rule;

            auto atom     = r::atom<number>;
            auto negate   = r::post_op_single<minus>(atom);
            auto addition = r::bin_op_single<plus>(negate);

            return r::expr(addition);
        }
    };

    struct visitor
    {
        int           result_of(P) const;
        constexpr int production(P, lex::static_token<number> num) const
        {
            return number::parse(num);
        }
        constexpr int production(P, int value, minus) const
        {
            return -value;
        }
        constexpr int production(P, int lhs, plus, int rhs) const
        {
            return lhs + rhs;
        }

        constexpr void error(lex::unexpected_token<grammar, P, number>,
                             const lex::tokenizer<test_spec>&) const
        {}
        constexpr void error(lex::illegal_operator_chain<grammar, P>,
                             const lex::tokenizer<test_spec>&) const
        {}
    };

    FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "4");
    verify(r0, 4);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r1 = parse<P>(visitor{}, "3-");
    verify(r1, -3);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "3 + 1");
    verify(r2, 4);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "3 + 1 + 2");
    verify(r3, unmatched);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r4 = parse<P>(visitor{}, "2--");
    verify(r4, unmatched);
}

TEST_CASE("operator_production: common atom")
{
    using grammar = lex::grammar<test_spec, struct P>;
    struct P : lex::operator_production<P, grammar>
    {
        static constexpr auto rule()
        {
            namespace r = lex::operator_rule;

            auto atom    = r::atom<number>;
            auto negate  = r::pre_op_single<minus>(atom);
            auto primary = r::expr(negate);

            auto math = r::bin_op_single<plus>(primary);
            auto bit  = r::bin_op_single<ampersand>(primary);

            return bit / math;
        }
    };

    struct visitor
    {
        int           result_of(P) const;
        constexpr int production(P, lex::static_token<number> num) const
        {
            return number::parse(num);
        }

        constexpr int production(P, minus, int number) const
        {
            return -number;
        }

        constexpr int production(P, int lhs, plus, int rhs) const
        {
            return lhs + rhs;
        }
        constexpr int production(P, int lhs, ampersand, int rhs) const
        {
            return static_cast<int>(static_cast<unsigned>(lhs) & static_cast<unsigned>(rhs));
        }

        constexpr void error(lex::unexpected_token<grammar, P, number>,
                             const lex::tokenizer<test_spec>&) const
        {}
        constexpr void error(lex::illegal_operator_chain<grammar, P>,
                             const lex::tokenizer<test_spec>&) const
        {}
    };

    FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "1");
    verify(r0, 1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r1 = parse<P>(visitor{}, "1 + -2");
    verify(r1, -1);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "1 & -1");
    verify(r2, 1);
}

TEST_CASE("operator_production: merge alternatives")
{
    using grammar = lex::grammar<test_spec, struct P>;
    struct P : lex::operator_production<P, grammar>
    {
        struct tag;

        static constexpr auto rule()
        {
            namespace r = lex::operator_rule;

            auto atom = r::atom<number> / r::parenthesized<paren_open, paren_close>;

            auto math_unary = r::pre_op_single<minus>(atom);
            auto math       = r::bin_op_single<plus>(math_unary);

            auto bit_unary = r::pre_op_single<tilde>(atom);
            auto bit       = r::bin_op_single<ampersand>(bit_unary);

            auto operation = r::expr<tag>(bit / math);

            auto comparison = r::bin_op_single<equal>(operation);

            return comparison;
        }
    };

    struct visitor
    {
        int           result_of(P) const;
        constexpr int production(P, lex::static_token<number> num) const
        {
            return number::parse(num);
        }

        constexpr int production(P, minus, int number) const
        {
            return -number;
        }
        constexpr int production(P, int lhs, plus, int rhs) const
        {
            return lhs + rhs;
        }

        constexpr int production(P, tilde, int number) const
        {
            return static_cast<int>(~static_cast<unsigned>(number));
        }
        constexpr int production(P, int lhs, ampersand, int rhs) const
        {
            return static_cast<int>(static_cast<unsigned>(lhs) & static_cast<unsigned>(rhs));
        }

        constexpr int production(P, int lhs, equal, int rhs) const
        {
            return lhs == rhs;
        }

        constexpr void error(lex::unexpected_token<grammar, P, number>,
                             const lex::tokenizer<test_spec>&) const
        {}
        constexpr void error(lex::unexpected_token<grammar, P, paren_close>,
                             const lex::tokenizer<test_spec>&) const
        {}
        constexpr void error(lex::illegal_operator_chain<grammar, P, P::tag>,
                             const lex::tokenizer<test_spec>&) const
        {}
    };

    FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "1");
    verify(r0, 1);

    SUBCASE("math")
    {
        FOONATHAN_LEX_TEST_CONSTEXPR auto r1 = parse<P>(visitor{}, "-1");
        verify(r1, -1);

        FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "1 + 2");
        verify(r2, 3);

        FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "-1 + 2");
        verify(r3, 1);

        FOONATHAN_LEX_TEST_CONSTEXPR auto r4 = parse<P>(visitor{}, "-1 +");
        verify(r4, unmatched);
    }
    SUBCASE("bit")
    {
        FOONATHAN_LEX_TEST_CONSTEXPR auto r1 = parse<P>(visitor{}, "~1");
        verify(r1, ~1);

        FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "6 & 3");
        verify(r2, 2);

        FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "6 & ~3");
        verify(r3, 4);

        FOONATHAN_LEX_TEST_CONSTEXPR auto r4 = parse<P>(visitor{}, "~1 &");
        verify(r4, unmatched);
    }
    SUBCASE("comparison")
    {
        FOONATHAN_LEX_TEST_CONSTEXPR auto r1 = parse<P>(visitor{}, "1 = 1");
        verify(r1, 1);

        FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "1 + 2 = 3");
        verify(r2, 1);

        FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "-1 + 3 = 6 & 3");
        verify(r3, 1);

        FOONATHAN_LEX_TEST_CONSTEXPR auto r4 = parse<P>(visitor{}, "1 = ");
        verify(r4, unmatched);
    }
    SUBCASE("parenthesized")
    {
        FOONATHAN_LEX_TEST_CONSTEXPR auto r1 = parse<P>(visitor{}, "1 + (1 + 1)");
        verify(r1, 3);

        FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "1 + -(1 + 1)");
        verify(r2, -1);

        FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "1 & (1 & 1)");
        verify(r3, 1);

        FOONATHAN_LEX_TEST_CONSTEXPR auto r4 = parse<P>(visitor{}, "1 & ~(1 & 1)");
        verify(r4, 0);

        FOONATHAN_LEX_TEST_CONSTEXPR auto r5 = parse<P>(visitor{}, "1 + (1 = 1)");
        verify(r5, 2);

        FOONATHAN_LEX_TEST_CONSTEXPR auto r6 = parse<P>(visitor{}, "1 + (1 = 1 & (1 = 0))");
        verify(r6, 1);
    }
    SUBCASE("illegal chain")
    {
        FOONATHAN_LEX_TEST_CONSTEXPR auto r1 = parse<P>(visitor{}, "1 + 1 + 2");
        verify(r1, unmatched);

        FOONATHAN_LEX_TEST_CONSTEXPR auto r2 = parse<P>(visitor{}, "1 & 1 & 1");
        verify(r2, unmatched);

        FOONATHAN_LEX_TEST_CONSTEXPR auto r3 = parse<P>(visitor{}, "1 = 1 = 1");
        verify(r3, unmatched);
    }
}

TEST_CASE("operator_production: finish")
{
    using grammar = lex::grammar<test_spec, struct P>;
    struct P : lex::operator_production<P, grammar>
    {
        static constexpr auto rule()
        {
            namespace r = lex::operator_rule;

            auto atom = r::atom<number>;
            auto add  = r::bin_op_single<plus>(atom);

            return add;
        }
    };

    struct visitor
    {
        unsigned result_of(P) const;

        constexpr unsigned production(P, lex::static_token<number> num) const
        {
            return static_cast<unsigned>(number::parse(num));
        }

        constexpr unsigned production(P, unsigned lhs, plus, unsigned rhs) const
        {
            return lhs + rhs;
        }

        constexpr int finish(P, unsigned result) const
        {
            return -static_cast<int>(result);
        }

        constexpr void error(lex::unexpected_token<grammar, P, number>,
                             const lex::tokenizer<test_spec>&) const
        {}
    };

    FOONATHAN_LEX_TEST_CONSTEXPR auto r0 = parse<P>(visitor{}, "4");
    verify(r0, -4);

    FOONATHAN_LEX_TEST_CONSTEXPR auto r1 = parse<P>(visitor{}, "1 + 3");
    verify(r1, -4);
}
