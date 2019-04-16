// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/lex/ascii.hpp>
#include <foonathan/lex/list_production.hpp>
#include <foonathan/lex/operator_production.hpp>
#include <foonathan/lex/parser.hpp>
#include <foonathan/lex/rule_production.hpp>
#include <foonathan/lex/token_production.hpp>

namespace grammar
{
namespace lex = foonathan::lex;

//=== tokens ===//
struct token_spec : lex::token_spec<struct whitespace, struct number, struct var, struct plus,
                                    struct minus, struct star, struct star_star, struct slash,
                                    struct tilde, struct pipe, struct ampersand, struct open_paren,
                                    struct close_paren, struct colon_eq, struct semicolon>
{};

struct whitespace : lex::rule_token<whitespace, token_spec>, lex::whitespace_token
{
    static constexpr auto rule() noexcept
    {
        return lex::token_rule::star(lex::ascii::is_space);
    }

    static constexpr const char* name = "<whitespace>";
};

struct number : lex::rule_token<number, token_spec>
{
    static constexpr auto rule() noexcept
    {
        return lex::token_rule::star(lex::ascii::is_digit);
    }

    static constexpr int parse(lex::static_token<number> number)
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

struct var : lex::rule_token<var, token_spec>
{
    static constexpr auto rule() noexcept
    {
        return lex::ascii::is_alpha;
    }

    static constexpr const char* name = "<var>";
};

struct plus : lex::literal_token<'+'>
{};
struct minus : lex::literal_token<'-'>
{};
struct star : lex::literal_token<'*'>
{};
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

struct open_paren : lex::literal_token<'('>
{};
struct close_paren : lex::literal_token<')'>
{};

struct colon_eq : FOONATHAN_LEX_LITERAL(":=")
{};
struct semicolon : lex::literal_token<';'>
{};

//=== productions ===//
struct grammar : lex::grammar<token_spec, struct decl_seq, struct decl, struct var_decl,
                              struct expr, struct atom_expr>
{};

struct atom_expr : lex::rule_production<atom_expr, grammar>
{
    static constexpr auto rule()
    {
        return number{} / var{};
    }
};

struct expr : lex::operator_production<expr, grammar>
{
    static constexpr auto rule()
    {
        namespace r = lex::operator_rule;
        auto atom   = r::atom<atom_expr> / r::parenthesized<open_paren, close_paren>;

        auto math_unary = r::pre_op_single<plus, minus>(atom);
        auto power      = r::bin_op_right<star_star>(math_unary);
        auto product    = r::bin_op_left<star, slash>(power);
        auto sum        = r::bin_op_left<plus, minus>(product);

        auto bit_unary = r::pre_op_single<tilde>(atom);
        auto bit_and   = r::bin_op_left<ampersand>(bit_unary);
        auto bit_or    = r::bin_op_left<pipe>(bit_and);

        return sum / bit_or + r::end;
    }
};

struct var_decl : lex::rule_production<var_decl, grammar>
{
    static constexpr auto rule()
    {
        namespace r = lex::production_rule;
        return var{} + r::silent<colon_eq> + expr{};
    }
};

struct decl : lex::rule_production<decl, grammar>
{
    static constexpr auto rule()
    {
        namespace r = lex::production_rule;
        return var{} + colon_eq{} >> var_decl{} | r::else_ >> expr{};
    }
};

struct decl_seq : lex::list_production<decl_seq, grammar, decl, semicolon>
{};
} // namespace grammar

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

int main()
{
    namespace lex = foonathan::lex;

    struct interpreter_t
    {
        int variables[256] = {};

        int operator()(grammar::atom_expr, lex::static_token<grammar::number> token)
        {
            return grammar::number::parse(token);
        }
        int operator()(grammar::atom_expr, lex::static_token<grammar::var> token)
        {
            return variables[token.spelling()[0]];
        }

        int operator()(lex::callback_result_of<grammar::expr>);
        int operator()(grammar::expr, int atom)
        {
            return atom;
        }
        int operator()(grammar::expr, grammar::plus, int rhs)
        {
            return rhs;
        }
        int operator()(grammar::expr, grammar::minus, int rhs)
        {
            return -rhs;
        }
        int operator()(grammar::expr, grammar::tilde, int rhs)
        {
            return ~unsigned(rhs);
        }
        int operator()(grammar::expr, int lhs, grammar::star_star, int rhs)
        {
            return static_cast<int>(std::pow(lhs, rhs));
        }
        int operator()(grammar::expr, int lhs, grammar::star, int rhs)
        {
            return lhs * rhs;
        }
        int operator()(grammar::expr, int lhs, grammar::slash, int rhs)
        {
            return lhs / rhs;
        }
        int operator()(grammar::expr, int lhs, grammar::plus, int rhs)
        {
            return lhs + rhs;
        }
        int operator()(grammar::expr, int lhs, grammar::minus, int rhs)
        {
            return lhs - rhs;
        }
        int operator()(grammar::expr, int lhs, grammar::ampersand, int rhs)
        {
            return unsigned(lhs) & unsigned(rhs);
        }
        int operator()(grammar::expr, int lhs, grammar::pipe, int rhs)
        {
            return unsigned(lhs) | unsigned(rhs);
        }

        int operator()(grammar::var_decl, lex::static_token<grammar::var> var, int value)
        {
            variables[var.spelling()[0]] = value;
            return value;
        }

        int operator()(grammar::decl, int value)
        {
            return value;
        }

        std::vector<int> operator()(grammar::decl_seq, int value)
        {
            return {value};
        }
        std::vector<int> operator()(grammar::decl_seq, std::vector<int>&& container, int value)
        {
            container.push_back(value);
            return std::move(container);
        }

        void operator()(
            lex::unexpected_token<grammar::grammar, grammar::atom_expr, grammar::number>,
            const lex::tokenizer<grammar::token_spec>& tokenizer)
        {
            std::cout << "error: expected number, got '" << tokenizer.peek().name() << "'\n";
        }
        void operator()(lex::unexpected_token<grammar::grammar, grammar::atom_expr, grammar::var>,
                        const lex::tokenizer<grammar::token_spec>& tokenizer)
        {
            std::cout << "error: expected variable, got '" << tokenizer.peek().name() << "'\n";
        }
        void operator()(lex::exhausted_token_choice<grammar::grammar, grammar::atom_expr,
                                                    grammar::number, grammar::var>,
                        const lex::tokenizer<grammar::token_spec>& tokenizer)
        {
            std::cout << "error: expected number or variable, got '" << tokenizer.peek().name()
                      << "'\n";
        }

        void operator()(
            lex::unexpected_token<grammar::grammar, grammar::expr, grammar::close_paren>,
            const lex::tokenizer<grammar::token_spec>& tokenizer)
        {
            std::cout << "error: expected ')', got '" << tokenizer.peek().name() << "'\n";
        }
        void operator()(lex::illegal_operator_chain<grammar::grammar, grammar::expr>,
                        const lex::tokenizer<grammar::token_spec>& tokenizer)
        {
            std::cout << "error: operator '" << tokenizer.peek().name()
                      << "' cannot be mixed with this expression\n";
        }

        void operator()(lex::unexpected_token<grammar::grammar, grammar::var_decl> error,
                        const lex::tokenizer<grammar::token_spec>&                 tokenizer)
        {
            std::cout << "error: expected '" << error.expected.name() << "', got '"
                      << tokenizer.peek().name() << "'\n";
        }

        void operator()(lex::exhausted_choice<grammar::grammar, grammar::decl>,
                        const lex::tokenizer<grammar::token_spec>& tokenizer)
        {
            std::cout << "error: expected expression or variable declaration, got '"
                      << tokenizer.peek().name() << "'\n";
        }

        void operator()(lex::unexpected_token<grammar::grammar, grammar::decl_seq, lex::eof_token>,
                        const lex::tokenizer<grammar::token_spec>& tokenizer)
        {
            std::cout << "error: expected eof, got '" << tokenizer.peek().name() << "'\n";
        }
    } interpreter;

    std::cout << "Simple calculator\n";

    std::string str;
    while (std::cout << "> ", std::getline(std::cin, str))
    {
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
