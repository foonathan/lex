// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

// This example implements an approximation of a tokenizer for C.

#include <foonathan/lex/ascii.hpp>     // utilities for ASCII matching
#include <foonathan/lex/tokenizer.hpp> // the main header for tokenization

// A namespace for the token grammar.
namespace C
{
namespace lex = foonathan::lex;

// Every token is a class inheriting from a special base that will specify the kind of token it is.
// The specification is then just an alias of `lex::token_spec` passing it all the tokens.
// As some token types need to refer to the token specification, we have to alias it first,
// and thus need to pass forward declarations.
//
// Note that the order of tokens in the specification doesn't matter.
using spec = lex::token_spec<
    struct whitespace, struct comment, struct identifier, struct int_literal, struct float_literal,
    struct char_literal, struct string_literal, struct auto_, struct break_, struct case_,
    struct char_, struct const_, struct continue_, struct default_, struct do_, struct double_,
    struct else_, struct enum_, struct extern_, struct float_, struct for_, struct goto_,
    struct if_, struct int_, struct long_, struct register_, struct return_, struct short_,
    struct signed_, struct sizeof_, struct switch_, struct typedef_, struct union_, struct void_,
    struct volatile_, struct while_, struct open_paren, struct close_paren, struct open_curly,
    struct close_curly, struct open_square, struct close_square, struct add, struct sub, struct mul,
    struct div, struct mod, struct and_, struct xor_, struct or_, struct shift_right,
    struct shift_left, struct inc, struct dec, struct assign, struct add_assign, struct sub_assign,
    struct mul_assign, struct div_assign, struct mod_assign, struct and_assign, struct xor_assign,
    struct or_assign, struct shift_right_assign, struct shift_left_assign, struct equal,
    struct not_equal, struct less, struct greater, struct less_equal, struct greater_equal,
    struct logical_and, struct logical_or, struct semicolon, struct comma, struct colon, struct dot,
    struct ellipsis, struct arrow, struct tilde, struct exclamation_mark, struct question_mark>;

//=== comments and whitespace ===//
// `lex::rule_token` is the base class for tokens that follow a more complex grammar rule.
// It is a CRTP base class and will invoke a `rule()` function of the derived class to determine
// whether a given input matches. This function returns a combination of `lex::token_rule` that are
// PEG (parsing expression grammar) rules.
//
// We also inherit from `lex::whitespace_token`.
// This means that the token will be skipped when iterating over all tokens later on.
//
// Note that rule tokens will be tried in arbitrary order, so they have to be mutually exclusive.
struct whitespace : lex::rule_token<whitespace, spec>, lex::whitespace_token
{
    static constexpr auto rule() noexcept
    {
        // Whitespace is just an arbitrary combination of ASCII whitespace characters.
        // Note that the token is only considered matched if there is at least one character
        // consumed, so in a top-level context `star()` and `plus()` are equivalent.
        return lex::token_rule::star(lex::ascii::is_space);
    }

    // We can also give the token a name, this is only required if you call `.name()` later on.
    static constexpr const char* name = "<whitespace>";
};

// Likewise, we can define a comment.
// It is also considered whitespace so will be skipped.
struct comment : lex::rule_token<comment, spec>, lex::whitespace_token
{
    // As comments start with `/` they conflict with the `/` literal.
    // So we have to tell the tokenizer that it has to check for a comment, after matching `/`.
    static constexpr bool is_conflicting_literal(token_kind kind) noexcept
    {
        return kind == token_kind::of<div>();
    }

    static constexpr auto rule() noexcept
    {
        namespace tr = lex::token_rule;

        // A C comment consists of `/*` followed by anything until `*/`.
        auto c_comment = "/*" + tr::until("*/");
        // A C++ comment consist of `//` followed by anything until a newline.
        // The newline is not considered part of this token.
        auto cpp_comment = "//" + tr::until_excluding(lex::ascii::is_newline);
        // A comment token is either a C or a C++ token.
        return c_comment / cpp_comment;
    }

    static constexpr const char* name = "<comment>";
};

//=== identifier ===//
// Identifiers are a special kind of `lex::rule_token`.
// They require some special interaction with keywords, so they inherit from `lex::identifier_token`
// instead. A token specification must contain at most one identifier token. Otherwise, they behave
// like `lex::rule_token`.
struct identifier : lex::identifier_token<identifier, spec>
{
    static constexpr auto rule() noexcept
    {
        namespace tr = lex::token_rule;

        // An identifier is a start character followed by zero-or-more rest characters.
        return is_identifier_start + tr::star(is_identifier_rest);
    }

    static constexpr bool is_identifier_start(char c) noexcept
    {
        return lex::ascii::is_alpha(c) || c == '_';
    }

    static constexpr bool is_identifier_rest(char c) noexcept
    {
        return lex::ascii::is_alnum(c) || c == '_';
    }

    // Note that we do not need to give an identifier token a name.
};

//=== literals ===/
// As int and float literals are tightly coupled, it would be nice if they could be parsed by one
// function. Here I choose to implement it in `float_literal`, so `int_literal` doesn't need to have
// any code for matching. As such it inherits from `lex::null_token`. Those tokens will not be
// matched alone, but only by other rules.
struct int_literal : lex::null_token
{
    static constexpr const char* name = "<int_literal>";
};

// Because `float_literal` matches both int and float literals,
// we have to write the matching code ourselves and cannot just provide a `rule()` function.
// So we inherit from `lex::basic_rule_token` instead, which is a similar CRTP class but it doesn't
// provide the matching implementation for us.
struct float_literal : lex::basic_rule_token<float_literal, spec>
{
    // As floating point numbers can start with `.`, they conflict with the `.` token.
    static constexpr bool is_conflicting_literal(token_kind kind) noexcept
    {
        return kind == token_kind::of<dot>();
    }

    // The rule for an integer suffix (i.e. the `u` in `0u`).
    static constexpr auto integer_suffix() noexcept
    {
        namespace tr = lex::token_rule;

        // Note the use of `tr::r`, `'u' / 'U'` wouldn't do the right thing.
        // so we have to manually turn one of the characters into a rule with that function.
        auto sign_suffix       = tr::r('u') / 'U';
        auto long_suffix       = tr::r('l') / 'L';
        auto sign_first_suffix = sign_suffix + tr::opt(long_suffix);
        auto long_first_suffix = long_suffix + tr::opt(sign_suffix);
        auto suffix            = sign_first_suffix / long_first_suffix;

        return tr::opt(suffix);
    }

    // The rule for a float suffix (i.e. the `f` in `0.f`).
    static constexpr auto float_suffix() noexcept
    {
        namespace tr = lex::token_rule;

        auto suffix = tr::r('f') / 'F' / 'l' / 'L';
        return tr::opt(suffix);
    }

    // The rule for a float exponent (i.e. `2e-4`).
    static constexpr auto float_exponent() noexcept
    {
        namespace tr = lex::token_rule;

        return (tr::r('e') / 'E') + tr::opt(tr::r('+') / '-') + tr::plus(is_decimal_digit);
    }

    // This is the function that must determine the match.
    // `str` points to the current position in the input, `end` one past the end.
    // It will only be called if there is at least one character.
    // It returns a `match_result` that describe which token was matched and how long it is.
    static constexpr match_result try_match(const char* str, const char* end) noexcept
    {
        namespace tr = lex::token_rule;

        // The rule for integer literals.
        // Note that '0' is an octal literal.
        auto hexadecimal = '0' + (tr::r('x') / 'X') + tr::plus(is_hexadecimal_digit);
        auto octal       = '0' + tr::star(is_octal_digit);
        // The `!tr::r('0')` is a negative lookahead - only if the next character is not `0`,
        // does the rule match.
        auto decimal  = !tr::r('0') + tr::plus(is_decimal_digit);
        auto int_rule = (hexadecimal / octal / decimal) + integer_suffix();

        // The rule for float literals.
        auto float_with_fraction
            = tr::padded(is_decimal_digit, '.', is_decimal_digit) + tr::opt(float_exponent());
        auto float_without_fraction = tr::plus(is_decimal_digit) + float_exponent();
        auto float_rule = (float_with_fraction / float_without_fraction) + float_suffix();

        // In order to match the rules, we create a `lex::rule_matcher` giving it the input.
        lex::rule_matcher<spec> matcher(str, end);
        if (matcher.match(float_rule))
            // It matched the floating point rule.
            // We finish parsing the float literal token if the remaining characters are not alpha
            // numeric. If the remaining characters are alpha numeric, an error token will be
            // matched instead. Note that we use a negative lookahead, so it will only look at the
            // next characters and never put them into the token.
            return matcher.finish(float_literal{}, !tr::r(lex::ascii::is_alnum) + !tr::r('_'));
        else if (matcher.match(int_rule))
            // It matched the integer rule token.
            // As above, we finish matching by checking
            return matcher.finish(int_literal{}, !tr::r(lex::ascii::is_alnum) + !tr::r('_'));
        else
            // It was neither an integer nor a float literal.
            return unmatched();
    }

    static constexpr bool is_decimal_digit(char c) noexcept
    {
        return lex::ascii::is_digit(c);
    }

    static constexpr bool is_octal_digit(char c) noexcept
    {
        return c >= '0' && c <= '7';
    }

    static constexpr bool is_hexadecimal_digit(char c) noexcept
    {
        return lex::ascii::is_digit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
    }

    static constexpr const char* name = "<float_literal>";
};

// A character literal is a boring rule token again.
struct char_literal : lex::rule_token<char_literal, spec>
{
    static constexpr auto rule() noexcept
    {
        namespace tr = lex::token_rule;

        auto octal_escape       = "\\0" + tr::at_most<2>(float_literal::is_octal_digit);
        auto hexadecimal_escape = "\\x" + tr::at_most<2>(float_literal::is_hexadecimal_digit);
        auto other_escape       = '\\' + tr::any; // for simplicity, allow any character

        auto c_char = octal_escape / hexadecimal_escape / other_escape / tr::any;
        return tr::opt('L') + '\'' + c_char + '\'';
    }

    static constexpr const char* name = "<char_literal>";
};

// And so is a string literal.
struct string_literal : lex::rule_token<string_literal, spec>
{
    static constexpr auto rule() noexcept
    {
        namespace tr = lex::token_rule;

        // If we have a \, we skip the character afterwards as well.
        // Otherwise, we only skip one character.
        // That way we will never match an escaped " and can just use `tr::until()` below.
        auto s_char = tr::if_then_else('\\', tr::any, tr::any);

        return tr::opt('L') + '"' + tr::until('"', s_char);
    }

    static constexpr const char* name = "<string_literal>";
};

//=== keywords ===//
// All keywords simply inherit from `FOONATHAN_LEX_KEYWORD(Str)`.
// This macro expands to `lex::keyword_token<Str[0], Str[1], ...>`.
// The tokenizer will automatically check whether an `lex::identifier_token` is actually a keyword,
// so if you have a single keyword token you also need to have an identifier token.
// Note that we don't need to name the tokens, they will be named automatically.

struct auto_ : FOONATHAN_LEX_KEYWORD("auto")
{};
struct break_ : FOONATHAN_LEX_KEYWORD("break")
{};
struct case_ : FOONATHAN_LEX_KEYWORD("case")
{};
struct char_ : FOONATHAN_LEX_KEYWORD("char")
{};
struct const_ : FOONATHAN_LEX_KEYWORD("const")
{};
struct continue_ : FOONATHAN_LEX_KEYWORD("continue")
{};
struct default_ : FOONATHAN_LEX_KEYWORD("default")
{};
struct do_ : FOONATHAN_LEX_KEYWORD("do")
{};
struct double_ : FOONATHAN_LEX_KEYWORD("double")
{};
struct else_ : FOONATHAN_LEX_KEYWORD("else")
{};
struct enum_ : FOONATHAN_LEX_KEYWORD("enum")
{};
struct extern_ : FOONATHAN_LEX_KEYWORD("extern")
{};
struct float_ : FOONATHAN_LEX_KEYWORD("float")
{};
struct for_ : FOONATHAN_LEX_KEYWORD("for")
{};
struct goto_ : FOONATHAN_LEX_KEYWORD("goto")
{};
struct if_ : FOONATHAN_LEX_KEYWORD("if")
{};
struct int_ : FOONATHAN_LEX_KEYWORD("int")
{};
struct long_ : FOONATHAN_LEX_KEYWORD("long")
{};
struct register_ : FOONATHAN_LEX_KEYWORD("register")
{};
struct return_ : FOONATHAN_LEX_KEYWORD("return")
{};
struct short_ : FOONATHAN_LEX_KEYWORD("short")
{};
struct signed_ : FOONATHAN_LEX_KEYWORD("signed")
{};
struct sizeof_ : FOONATHAN_LEX_KEYWORD("sizeof")
{};
struct static_ : FOONATHAN_LEX_KEYWORD("static")
{};
struct struct_ : FOONATHAN_LEX_KEYWORD("struct")
{};
struct switch_ : FOONATHAN_LEX_KEYWORD("switch")
{};
struct typedef_ : FOONATHAN_LEX_KEYWORD("typedef")
{};
struct union_ : FOONATHAN_LEX_KEYWORD("union")
{};
struct unsigned_ : FOONATHAN_LEX_KEYWORD("unsigned")
{};
struct void_ : FOONATHAN_LEX_KEYWORD("void")
{};
struct volatile_ : FOONATHAN_LEX_KEYWORD("volatile")
{};
struct while_ : FOONATHAN_LEX_KEYWORD("while")
{};

//=== punctuation tokens ===//
// Punctuation tokens are similar to keyword tokens but do not need the special interaction with
// identifier tokens. So they inherit from `FOONATHAN_LEX_LITERAL(Str)` or
// `lex::literal_token<Str[0], Str[1], ...>`. The tokenizer will automatically match them after it
// tried all rule tokens unsuccessfully.
//
// Note that the order doesn't matter, it will match the longest token possible.

struct open_paren : FOONATHAN_LEX_LITERAL("(")
{};
struct close_paren : FOONATHAN_LEX_LITERAL(")")
{};
struct open_curly : FOONATHAN_LEX_LITERAL("{")
{};
struct close_curly : FOONATHAN_LEX_LITERAL("}")
{};
struct open_square : FOONATHAN_LEX_LITERAL("[")
{};
struct close_square : FOONATHAN_LEX_LITERAL("]")
{};

struct add : FOONATHAN_LEX_LITERAL("+")
{};
struct sub : FOONATHAN_LEX_LITERAL("-")
{};
struct mul : FOONATHAN_LEX_LITERAL("*")
{};
struct div : FOONATHAN_LEX_LITERAL("/")
{};
struct mod : FOONATHAN_LEX_LITERAL("%")
{};
struct and_ : FOONATHAN_LEX_LITERAL("&")
{};
struct xor_ : FOONATHAN_LEX_LITERAL("^")
{};
struct or_ : FOONATHAN_LEX_LITERAL("|")
{};
struct shift_right : FOONATHAN_LEX_LITERAL(">>")
{};
struct shift_left : FOONATHAN_LEX_LITERAL("<<")
{};

struct inc : FOONATHAN_LEX_LITERAL("++")
{};
struct dec : FOONATHAN_LEX_LITERAL("--")
{};

struct assign : FOONATHAN_LEX_LITERAL("=")
{};
struct add_assign : FOONATHAN_LEX_LITERAL("+=")
{};
struct sub_assign : FOONATHAN_LEX_LITERAL("-=")
{};
struct mul_assign : FOONATHAN_LEX_LITERAL("*=")
{};
struct div_assign : FOONATHAN_LEX_LITERAL("/=")
{};
struct mod_assign : FOONATHAN_LEX_LITERAL("%=")
{};
struct and_assign : FOONATHAN_LEX_LITERAL("&=")
{};
struct xor_assign : FOONATHAN_LEX_LITERAL("^=")
{};
struct or_assign : FOONATHAN_LEX_LITERAL("|=")
{};
struct shift_right_assign : FOONATHAN_LEX_LITERAL(">>=")
{};
struct shift_left_assign : FOONATHAN_LEX_LITERAL("<<=")
{};

struct equal : FOONATHAN_LEX_LITERAL("==")
{};
struct not_equal : FOONATHAN_LEX_LITERAL("!=")
{};
struct less : FOONATHAN_LEX_LITERAL("<")
{};
struct greater : FOONATHAN_LEX_LITERAL(">")
{};
struct less_equal : FOONATHAN_LEX_LITERAL("<=")
{};
struct greater_equal : FOONATHAN_LEX_LITERAL(">=")
{};
struct logical_and : FOONATHAN_LEX_LITERAL("&&")
{};
struct logical_or : FOONATHAN_LEX_LITERAL("||")
{};

struct semicolon : FOONATHAN_LEX_LITERAL(";")
{};
struct comma : FOONATHAN_LEX_LITERAL(",")
{};
struct colon : FOONATHAN_LEX_LITERAL(":")
{};
struct dot : FOONATHAN_LEX_LITERAL(".")
{};
struct ellipsis : FOONATHAN_LEX_LITERAL("...")
{};
struct arrow : FOONATHAN_LEX_LITERAL("->")
{};
struct tilde : FOONATHAN_LEX_LITERAL("~")
{};
struct exclamation_mark : FOONATHAN_LEX_LITERAL("!")
{};
struct question_mark : FOONATHAN_LEX_LITERAL("?")
{};
} // namespace C

#if !defined(FOONATHAN_LEX_TEST)

// A simple driver program that tokenizes the standard input.

#    include <iostream>
#    include <string>

int main()
{
    namespace lex = foonathan::lex;

    // We need to read the input a string.
    // This is required because the tokens are just string views into that string.
    std::string input(std::istreambuf_iterator<char>{std::cin}, std::istreambuf_iterator<char>{});

    // Create a tokenizer for C passing it the string as input.
    lex::tokenizer<C::spec> tokenizer(input.c_str(), input.size());
    // As long as we still have characters left.
    while (!tokenizer.is_done())
    {
        // Match and consume the current token.
        auto token = tokenizer.get();

        if (token.is_category<lex::is_literal_token>())
            // If it is a literal token, just print the spelling.
            std::cout << '`' << std::string(token.spelling().data(), token.spelling().size())
                      << "`\n";
        else
            // Otherwise, print the name and the spelling.
            std::cout << token.name() << ": `"
                      << std::string(token.spelling().data(), token.spelling().size()) << "`\n";
    }
}

#else

// Unit tests for the tokenizer.

#    include <catch.hpp>

#    include <tokenize.hpp>

namespace
{
void check_token(lex::token<C::spec> token, lex::token_kind<C::spec> kind, const char* spelling)
{
    INFO(std::string(token.spelling().data(), token.spelling().size()) << ": " << token.name());
    REQUIRE(token.spelling() == spelling);
    REQUIRE(token.kind() == kind);
}
} // namespace

TEST_CASE("whitespace and comments")
{
    static constexpr const char       array[]   = R"(int int/* C comment */

int // C++ comment
int
)";
    constexpr auto                    tokenizer = lex::tokenizer<C::spec>(array);
    FOONATHAN_LEX_TEST_CONSTEXPR auto result    = tokenize<C::spec>(tokenizer);

    REQUIRE(result.size() == 4);
    check_token(result[0], C::int_{}, "int");
    check_token(result[1], C::int_{}, "int");
    check_token(result[2], C::int_{}, "int");
    check_token(result[3], C::int_{}, "int");
}

TEST_CASE("identifier and keywords")
{
    static constexpr const char       array[]   = R"(
int
integer
Foo_bar123
__reserved
12anumber
)";
    constexpr auto                    tokenizer = lex::tokenizer<C::spec>(array);
    FOONATHAN_LEX_TEST_CONSTEXPR auto result    = tokenize<C::spec>(tokenizer);

    REQUIRE(result.size() == 6);
    check_token(result[0], C::int_{}, "int");
    check_token(result[1], C::identifier{}, "integer");
    check_token(result[2], C::identifier{}, "Foo_bar123");
    check_token(result[3], C::identifier{}, "__reserved");
    check_token(result[4], lex::error_token{}, "12");
    check_token(result[5], C::identifier{}, "anumber");
}

TEST_CASE("int and float literals")
{
    static constexpr const char       array[]   = R"(
1234567890
0x1234567890ABCDEFabcdefl
0X42LU
01234567
0u
.123
1.23
123e45f
1.23E-4
1.
09
)";
    constexpr auto                    tokenizer = lex::tokenizer<C::spec>(array);
    FOONATHAN_LEX_TEST_CONSTEXPR auto result    = tokenize<C::spec>(tokenizer);

    REQUIRE(result.size() == 12);
    check_token(result[0], C::int_literal{}, "1234567890");
    check_token(result[1], C::int_literal{}, "0x1234567890ABCDEFabcdefl");
    check_token(result[2], C::int_literal{}, "0X42LU");
    check_token(result[3], C::int_literal{}, "01234567");
    check_token(result[4], C::int_literal{}, "0u");
    check_token(result[5], C::float_literal{}, ".123");
    check_token(result[6], C::float_literal{}, "1.23");
    check_token(result[7], C::float_literal{}, "123e45f");
    check_token(result[8], C::float_literal{}, "1.23E-4");
    check_token(result[9], C::float_literal{}, "1.");
    check_token(result[10], lex::error_token{}, "0");
    check_token(result[11], C::int_literal{}, "9");
}

TEST_CASE("string and char literals")
{
    static constexpr const char       array[]   = R"(
'a'
'\n'
L'\''
'\0'
'\x42'
"hello world!"
L"hello \"world\""
)";
    constexpr auto                    tokenizer = lex::tokenizer<C::spec>(array);
    FOONATHAN_LEX_TEST_CONSTEXPR auto result    = tokenize<C::spec>(tokenizer);

    REQUIRE(result.size() == 7);
    check_token(result[0], C::char_literal{}, R"('a')");
    check_token(result[1], C::char_literal{}, R"('\n')");
    check_token(result[2], C::char_literal{}, R"(L'\'')");
    check_token(result[3], C::char_literal{}, R"('\0')");
    check_token(result[4], C::char_literal{}, R"('\x42')");
    check_token(result[5], C::string_literal{}, R"("hello world!")");
    check_token(result[6], C::string_literal{}, R"(L"hello \"world\"")");
}

#endif
