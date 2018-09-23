// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_BM_TRIE_HPP_INCLUDED
#define FOONATHAN_LEX_BM_TRIE_HPP_INCLUDED

#include <foonathan/lex/ascii.hpp>
#include <foonathan/lex/tokenizer.hpp>

namespace trie_ns
{
foonathan::lex::token_spelling bump(const char*& str, std::size_t n)
{
    auto spelling = foonathan::lex::token_spelling(str, n);
    str += n;
    return spelling;
}

namespace lex = foonathan::lex;

using literals = lex::token_spec<struct ellipsis, struct dot, struct plus_eq, struct plus_plus,
                                 struct plus, struct arrow_deref, struct arrow, struct minus_minus,
                                 struct minus_eq, struct minus, struct tilde>;

struct ellipsis : lex::literal_token<'.', '.', '.'>
{};

struct dot : lex::literal_token<'.'>
{};

struct plus_eq : lex::literal_token<'+', '='>
{};

struct plus_plus : lex::literal_token<'+', '+'>
{};

struct plus : lex::literal_token<'+'>
{};

struct arrow_deref : lex::literal_token<'-', '>', '*'>
{};

struct arrow : lex::literal_token<'-', '>'>
{};

struct minus_minus : lex::literal_token<'-', '-'>
{};

struct minus_eq : lex::literal_token<'-', '='>
{};

struct minus : lex::literal_token<'-'>
{};

struct tilde : lex::literal_token<'~'>
{};
} // namespace trie_ns

void trie(const char* str, const char* end, void (*f)(int, foonathan::lex::token_spelling))
{
    using namespace trie_ns;
    namespace lex = foonathan::lex;

    using trie = lex::detail::literal_trie<literals, literals>;
    while (str != end)
    {
        auto result = trie::lookup_prefix(str, end);
        if (result)
            f(result.data, bump(str, result.length));
        else if (lex::ascii::is_space(*str))
        {
            auto begin = str++;
            while (str != end && lex::ascii::is_space(*str))
                ++str;
            f(-1, lex::token_spelling(begin, static_cast<std::size_t>(str - begin)));
        }
        else
            ++str;
    }
}

#endif // FOONATHAN_LEX_BM_TRIE_HPP_INCLUDED
