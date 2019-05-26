// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

// This example implements a calculator REPL.

#include <foonathan/lex/ascii.hpp>               // utilities for ASCII matching
#include <foonathan/lex/list_production.hpp>     // for the list production
#include <foonathan/lex/operator_production.hpp> // for operator productions
#include <foonathan/lex/parser.hpp>              // for parsing productions
#include <foonathan/lex/rule_production.hpp>     // for complex rule productions

// A namespace for the grammar of the calculator.
namespace grammar
{
namespace lex = foonathan::lex;

//=== tokens ===//
// The tokens, see example/ctokenizer.cpp for details.
struct token_spec : lex::token_spec<struct whitespace, struct number, struct var, struct plus,
                                    struct minus, struct star, struct star_star, struct slash,
                                    struct tilde, struct pipe, struct ampersand, struct open_paren,
                                    struct close_paren, struct colon_eq, struct semicolon>
{};

// We're ignoring arbitrary whitespace.
struct whitespace : lex::rule_token<whitespace, token_spec>, lex::whitespace_token
{
    static constexpr auto rule() noexcept
    {
        return lex::token_rule::star(lex::ascii::is_space);
    }

    static constexpr const char* name = "<whitespace>";
};

// A number which is a sequence of digits.
struct number : lex::rule_token<number, token_spec>
{
    static constexpr auto rule() noexcept
    {
        return lex::token_rule::star(lex::ascii::is_digit);
    }

    // When parsing, every token can have an optionally `parse()` function.
    // The parse callback will then receive the result of this function as well.
    //
    // Here we're converting the token into the number it represents.
    // (Note that the base class also defines a `token` as an alias for `lex::token<token_spec>`.
    static constexpr int parse(token number)
    {
        int result = 0;
        for (auto ptr = number.spelling().end(); ptr != number.spelling().begin(); --ptr)
        {
            auto c     = ptr[-1];
            auto digit = c - '0';
            result *= 10;
            result += digit;
        }
        return result;
    }

    static constexpr const char* name = "<number>";
};

// A variable which is just a single letter.
struct var : lex::rule_token<var, token_spec>
{
    static constexpr auto rule() noexcept
    {
        return lex::ascii::is_alpha;
    }

    // The parse function just takes the first character.
    static constexpr char parse(token var)
    {
        return var.spelling()[0];
    }

    static constexpr const char* name = "<var>";
};

// The operators.
struct plus : lex::literal_token<'+'>
{};
struct minus : lex::literal_token<'-'>
{};
struct star : lex::literal_token<'*'>
{};
// Instead of the macro we could have also written `literal_token<'*', '*'>`
struct star_star : FOONATHAN_LEX_LITERAL("**")
{};
struct slash : lex::literal_token<'/'>
{};
struct tilde : lex::literal_token<'~'>
{};
struct ampersand : lex::literal_token<'&'>
{};
struct pipe : lex::literal_token<'|'>
{};

// Parenthesis.
struct open_paren : lex::literal_token<'('>
{};
struct close_paren : lex::literal_token<')'>
{};

// Other punctation.
struct colon_eq : FOONATHAN_LEX_LITERAL(":=")
{};
struct semicolon : lex::literal_token<';'>
{};

//=== productions ===//
// While `lex::token_spec` lists all tokens, `lex::grammar` specifies the tokens and lists all
// productions. The first production - `decl_seq` in this case - is the start production, all
// parsing will start there.
struct grammar : lex::grammar<token_spec, struct decl_seq, struct decl, struct var_decl,
                              struct expr, struct atom_expr>
{};

// An atom expression is an atomic operand (number or variable).
// It is a `rule_production` - a production defined by a rule.
// Similar to `rule_token` it has to define a `rule()` function that returns the rule.
struct atom_expr : lex::rule_production<atom_expr, grammar>
{
    static constexpr auto rule()
    {
        // It is either the `number` or the `var` token.
        // The operator `/` means "token alternative".
        // It can only be used to alternate between tokens.
        // Choices are made using single token lookahead.
        return number{} / var{};
    }
};

// An arbitrary expression.
// It is an `operator_production` as it consists of operators and operands.
// The `rule()` function defines the details.
//
// Our calculator should support the mathematical operators with usual precedences (e.g. `1 + 2 *
// -3`), and bitwise operators with their usual precedences (e.g. `1 | 2 & ~3`). But I don't want to
// assign relative precedences to math and bitwise operators, instead parentheses should be required
// (e.g. `1 + 2 & 3` is ill-formed, either `(1 + 2) & 3` or `1 + (2 & 3)` has to be used).
struct expr : lex::operator_production<expr, grammar>
{
    static constexpr auto rule()
    {
        // Convenience namespace alias.
        namespace r = lex::operator_rule;

        // An atomic expression, which is the final operand.
        // It is either the atomic production - `atom_expr` in this case,
        // or an arbitrary parenthesized expression where `open_paren` and `close_paren` are the
        // parenthesis tokens. (The `/` here is only valid in this exact context).
        auto atom = r::atom<atom_expr> / r::parenthesized<open_paren, close_paren>;

        // The mathematical operator hierarchy.
        // A math unary expressions is a single unary prefix operator ('+' or '-') followed by an
        // atom.
        auto math_unary = r::pre_op_single<plus, minus>(atom);
        // A pow expression (`2 ** 2`) is a binary operator (`**`) that can be chained.
        // It is right associative and the operands are `math_unary` expressions.
        auto power = r::bin_op_right<star_star>(math_unary);
        // A product is a binary operator ('*' or '/') that can be chained.
        // It is left associative and the operands are `power` expressions.
        auto product = r::bin_op_left<star, slash>(power);
        // A sum is a binary operator ('+' or '-') that can be chained.
        // It is right associative and the operands are `product` expressions.
        auto sum = r::bin_op_left<plus, minus>(product);

        // The bit operator hierarchy.
        // Again, a unary prefix operator.
        auto bit_unary = r::pre_op_single<tilde>(atom);
        // And again, two binary operators that are left associative.
        auto bit_and = r::bin_op_left<ampersand>(bit_unary);
        auto bit_or  = r::bin_op_left<pipe>(bit_and);

        // The full expressions is either the math or bitwise hierarchy.
        // This is specified by `sum / bit_or`, i.e. we have either a sum or a bit or.
        // The choice is made with single token look-ahead, if an operator would be valid for both
        // `sum` and `bit_or`, the first one is selected.
        //
        // If we had `sum / bit_or` alone, parsing `1 + 2 & 3` would read `1 + 2` and leave `& 2` in
        // the stream, as it does not fit into a sum. The `r::expr` turns that into an error
        // instead. To be more precise, if the next token is *any* operator of the expression, it
        // results in an error.
        return r::expr(sum / bit_or);
    }
};

// A variable declaration is a rule again.
struct var_decl : lex::rule_production<var_decl, grammar>
{
    static constexpr auto rule()
    {
        namespace r = lex::production_rule;
        // The syntax is `var x := 4`.
        // This is specified as a sequence using `+`.
        // The `r::silent` means that the `:=` token will not be passed to the visitor.
        return var{} + r::silent<colon_eq> + expr{};
    }
};

// A declaration is also a rule.
// It is either a variable declaration or an expression.
struct decl : lex::rule_production<decl, grammar>
{
    static constexpr auto rule()
    {
        namespace r = lex::production_rule;

        // `operator/` can only be used with tokens but here we have productions.
        // So we need to use `operator|`. Unlike `operator/` it has to be the final expression, it
        // cannot be combined with e.g. an additional `+`. The library itself does not do
        // backtracking, so we have to give conditions when each branch should be taken.
        //
        // We know that `:=` after a name means we have a variable declaration,
        // this is specified using `var{} + colon_eq{} >>`.
        // If these tokens are next, it will not consume them but instead parse `var_decl{}`.
        //
        // `r::else_` is a condition taken if no other matches.
        // The branches will be tried in order.
        return var{} + colon_eq{} >> var_decl{} | r::else_ >> expr{};
    }
};

// A declaration sequence are multiple declarations seperated by `;`.
// We specify that using a `list_production`.
struct decl_seq : lex::list_production<decl_seq, grammar>
{
    // This is the element of the list, it is required.
    using element = decl;
    // This is the separator between elements, it is also required.
    using separator_token = semicolon;

    // We want to allow trailing seperators (by default it is forbidden).
    using allow_trailing = std::true_type;
    // But to determine whether a separator is trailing, the parser has to know which token is
    // *after* the list. This is the `end_token`, if we have a separator and then the end token, the
    // list ends. The end token is not consumed.
    //
    // As `decl_seq` is the initial production the end token is EOF.
    using end_token = lex::eof_token;
};
} // namespace grammar

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

int main()
{
    namespace lex = foonathan::lex;

    // A parser visitor that will interpret the input.
    //
    // Upon parsing a production it will call the `production()` overload that accepts the sequence
    // that was parsed. The result will be used as the argument for productions that have it as
    // sub-productions. Here, the result is the value of the expression.
    struct interpreter_t
    {
        // We have one variable for each character.
        int variables[256] = {};

        // For tokens it is called passing it a `static_token`, which also contains the result of
        // the `::parse()` function of the token. We have either a `number` or a `var`, which means
        // two overloads.
        int production(grammar::atom_expr, lex::static_token<grammar::number, int> number)
        {
            // the value of a number is just the number itself
            return number.value();
        }
        int production(grammar::atom_expr, lex::static_token<grammar::var, char> var)
        {
            // The value of a variable is read.
            return variables[var.value()];
        }

        // For recursive productions like `expr` we need this special overload.
        // It does not need to be defined, it is just there to designate the return type of that
        // production.
        int result_of(grammar::expr);
        int production(grammar::expr, int atom) // atomic expression
        {
            return atom;
        }
        // Instead of `static_token<T>` we can also use `T` itself, it implicitly converts.
        int production(grammar::expr, grammar::plus, int rhs) // +rhs
        {
            return rhs;
        }
        int production(grammar::expr, grammar::minus, int rhs) // -rhs
        {
            return -rhs;
        }
        int production(grammar::expr, grammar::tilde, int rhs) // ~rhs
        {
            return ~unsigned(rhs);
        }
        int production(grammar::expr, int lhs, grammar::star_star, int rhs) // lhs ** rhs
        {
            return static_cast<int>(std::pow(lhs, rhs));
        }
        int production(grammar::expr, int lhs, grammar::star, int rhs) // lhs * rhs
        {
            return lhs * rhs;
        }
        int production(grammar::expr, int lhs, grammar::slash, int rhs) // lhs / rhs
        {
            return lhs / rhs;
        }
        int production(grammar::expr, int lhs, grammar::plus, int rhs) // lhs + rhs
        {
            return lhs + rhs;
        }
        int production(grammar::expr, int lhs, grammar::minus, int rhs) // lhs - rhs
        {
            return lhs - rhs;
        }
        int production(grammar::expr, int lhs, grammar::ampersand, int rhs) // lhs & rhs
        {
            return unsigned(lhs) & unsigned(rhs);
        }
        int production(grammar::expr, int lhs, grammar::pipe, int rhs) // lhs | rhs
        {
            return unsigned(lhs) | unsigned(rhs);
        }

        // The value of a variable declaration is the initializing value.
        // The `:=` token was `silent`, so it will not be passed to the callback.
        // The callback of an expression returns an `int`, so that is the final argument.
        int production(grammar::var_decl, lex::static_token<grammar::var, char> var, int value)
        {
            variables[var.value()] = value;
            return value;
        }

        // A declaration is either a variable or an expression.
        // Both return an `int`, so that is the argument.
        int production(grammar::decl, int value)
        {
            return value;
        }

        // Note that not all productions have to return the same type.
        // Just all overloads of a single production.
        // For a list production (that is non-empty), we need one initialing overload...
        std::vector<int> production(grammar::decl_seq, int value)
        {
            // which creates a "container" of size 1
            return {value};
        }
        // ... and one combining overload.
        std::vector<int> production(grammar::decl_seq, std::vector<int>&& container, int value)
        {
            // which adds the new value to the "container"
            container.push_back(value);
            return std::move(container);
        }

        // Error messages are also reported by calling appropriate overloads.
        // They all receive an error type and the tokenizer.
        // The error type has the grammar as first argument, the active production as second,
        // followed by optional arguments.
        // All but the grammar can be type-erased, that way multiple errors of the same kind can be
        // handled with one overload.
        //
        // An `exhausted_token_choice` happens when there was an `a / b` and the token was neither
        // `a` nor `b`. Here is the error handler for the `number{} / var{}` of our atomic
        // expression.
        void error(lex::exhausted_token_choice<grammar::grammar, grammar::atom_expr,
                                               grammar::number, grammar::var>,
                   const lex::tokenizer<grammar::token_spec>& tokenizer)
        {
            print_line(tokenizer);
            std::cout << "error: expected number or variable, got '" << tokenizer.peek().name()
                      << "'\n";
        }

        // An `unexpected_token` happens when we expected an `a` but did not get an `a`.
        // Here is the error when have something like `(1 + 2;` (note the missing `)`).
        void error(lex::unexpected_token<grammar::grammar, grammar::expr, grammar::close_paren>,
                   const lex::tokenizer<grammar::token_spec>& tokenizer)
        {
            print_line(tokenizer);
            std::cout << "error: expected ')', got '" << tokenizer.peek().name() << "'\n";
        }
        // An `illegal_operator_chain` is the error reported by the operator rule `r::end` when
        // we've combined operators we must not combine. For example, writing `1 + 2 & 3` will
        // trigger it.
        void error(lex::illegal_operator_chain<grammar::grammar, grammar::expr> error,
                   const lex::tokenizer<grammar::token_spec>&                   tokenizer)
        {
            print_line(tokenizer);
            std::cout << "error: operator '" << tokenizer.peek().name()
                      << "' cannot be mixed with operator '" << error.op.name() << "'\n";
        }

        // Again, an unexpected token, but note that we've type-erased *which* token was expected.
        // That way in `a := 4` we can handle both a missing `a` and `:=`.
        // (Due to the way `var_decl` is used the error cannot actually happen but the overload
        // can't know that).
        void error(lex::unexpected_token<grammar::grammar, grammar::var_decl> error,
                   const lex::tokenizer<grammar::token_spec>&                 tokenizer)
        {
            print_line(tokenizer);
            std::cout << "error: expected '" << error.expected.name() << "', got '"
                      << tokenizer.peek().name() << "'\n";
        }

        // Similar to an `exhausted_token_choice,` `exhausted_choice` is when we had `a | b` but
        // neither `a` nor `b` worked. For implementation reasons it cannot give the actual tokens
        // that were expected.
        void error(lex::exhausted_choice<grammar::grammar, grammar::decl>,
                   const lex::tokenizer<grammar::token_spec>& tokenizer)
        {
            print_line(tokenizer);
            std::cout << "error: expected expression or variable declaration, got '"
                      << tokenizer.peek().name() << "'\n";
        }

        // This error is created because `decl_seq` is the initial production.
        // The initial production must consume all input, if that is not the case, this error is
        // triggered.
        void error(lex::unexpected_token<grammar::grammar, grammar::decl_seq, lex::eof_token>,
                   const lex::tokenizer<grammar::token_spec>& tokenizer)
        {
            print_line(tokenizer);
            std::cout << "error: expected eof, got '" << tokenizer.peek().name() << "'\n";
        }

    private:
        // This is just prints an error marker below the input.
        void print_line(const lex::tokenizer<grammar::token_spec>& tokenizer)
        {
            auto offset = tokenizer.current_ptr() - tokenizer.begin_ptr();
            auto spaces = offset + 2; // to account for "> " at beginning
            std::cout << std::string(spaces, ' ') << "^\n";
        }
    } interpreter;

    std::cout << "Simple calculator\n";

    // Now we just have a simple REPL.
    std::string str;
    while (std::cout << "> ", std::getline(std::cin, str))
    {
        // We're parsing one line at a time, passing our interpreter.
        // The result will be a `lex::parse_result<std::vector<int>>`.
        // The `vector` contains the result of all declarations but only if parsing was at least
        // partially successful.
        // For example, an input of `1 + 2 3` is valid (`1 + 2`) but just does not consume
        // everything. This means that the `unexpected_token<grammar, decl_seq, eof>` is triggered,
        // but we still have a parse result containing `{3}`.
        auto result = lex::parse<grammar::grammar>(str.data(), str.size(), interpreter);
        if (result.is_success())
        {
            auto first = true;
            for (auto val : result.value())
            {
                if (first)
                    first = false;
                else
                    std::cout << "; ";
                std::cout << val;
            }
            std::cout << '\n';
        }
    }
}
