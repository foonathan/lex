// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_BM_TOKENIZER_MANUAL_HPP_INCLUDED
#define FOONATHAN_LEX_BM_TOKENIZER_MANUAL_HPP_INCLUDED

#include <foonathan/lex/ascii.hpp>
#include <foonathan/lex/tokenizer.hpp>

namespace tokenizer_manual_ns
{
namespace lex = foonathan::lex;

template <unsigned N>
bool starts_with(const char* str, const char* end, const char (&token)[N])
{
    auto remaining = unsigned(end - str);
    if (remaining < N - 1)
        return false;
    else
        return std::strncmp(str, token, N - 1) == 0;
}

using token_spec
    = lex::token_spec<struct ellipsis, struct dot, struct plus_eq, struct plus_plus, struct plus,
                      struct arrow_deref, struct arrow, struct minus_minus, struct minus_eq,
                      struct minus, struct tilde, struct whitespace>;

struct ellipsis : lex::null_token
{};

struct dot : lex::rule_token<dot, token_spec>
{
    static match_result try_match(const char* str, const char* end)
    {
        if (starts_with(str, end, "..."))
            return ok<ellipsis>(3);
        else if (starts_with(str, end, "."))
            return ok<dot>(1);
        else
            return unmatched();
    }
};

struct plus_eq : lex::null_token
{};

struct plus_plus : lex::null_token
{};

struct plus : lex::rule_token<plus, token_spec>
{
    static match_result try_match(const char* str, const char* end)
    {
        if (starts_with(str, end, "+="))
            return ok<plus_eq>(2);
        else if (starts_with(str, end, "++"))
            return ok<plus_plus>(2);
        else if (starts_with(str, end, "+"))
            return ok<plus>(1);
        else
            return unmatched();
    }
};

struct arrow_deref : lex::null_token
{};

struct arrow : lex::null_token
{};

struct minus_minus : lex::null_token
{};

struct minus_eq : lex::null_token
{};

struct minus : lex::rule_token<minus, token_spec>
{
    static match_result try_match(const char* str, const char* end)
    {
        if (starts_with(str, end, "->*"))
            return ok<arrow_deref>(3);
        else if (starts_with(str, end, "->"))
            return ok<arrow>(2);
        else if (starts_with(str, end, "--"))
            return ok<minus_minus>(2);
        else if (starts_with(str, end, "-"))
            return ok<minus_minus>(1);
        else
            return unmatched();
    }
};

struct tilde : lex::rule_token<tilde, token_spec>
{
    static match_result try_match(const char* str, const char* end)
    {
        if (starts_with(str, end, "~"))
            return ok(1);
        else
            return unmatched();
    }
};

struct whitespace : lex::rule_token<whitespace, token_spec>,
                    lex::loop_ascii_mixin<whitespace, lex::ascii::is_space>
{};
} // namespace tokenizer_manual_ns

void tokenizer_manual(const char* str, const char* end,
                      void (*f)(int, foonathan::lex::token_spelling))
{
    using namespace tokenizer_manual_ns;
    namespace lex = foonathan::lex;

    lex::tokenizer<token_spec> tokenizer(str, end);
    while (!tokenizer.is_eof())
    {
        auto cur = tokenizer.peek();
        if (!cur.is_error())
            f(cur.kind().get(), cur.spelling());
        tokenizer.bump();
    }
}
#endif // FOONATHAN_LEX_BM_TOKENIZER_MANUAL_HPP_INCLUDED
