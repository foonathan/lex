// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_PARSER_HPP_INCLUDED
#define FOONATHAN_LEX_PARSER_HPP_INCLUDED

#include <foonathan/lex/grammar.hpp>
#include <foonathan/lex/parse_error.hpp>
#include <foonathan/lex/parse_result.hpp>
#include <foonathan/lex/tokenizer.hpp>

namespace foonathan
{
namespace lex
{
    template <class Grammar, class Func>
    constexpr auto parse(tokenizer<typename Grammar::token_spec>& tokenizer, Func&& f)
    {
        auto result = Grammar::start::parse(tokenizer, f);
        if (result.is_success() && !tokenizer.is_done())
        {
            unexpected_token<Grammar, typename Grammar::start, eof_token>
                error(typename Grammar::start{}, eof_token{});
            detail::report_error(f, error, tokenizer);
        }
        return result;
    }

    template <class Grammar, class Func>
    constexpr auto parse(const char* str, std::size_t size, Func&& f)
    {
        tokenizer<typename Grammar::token_spec> tok(str, size);
        return parse<Grammar>(tok, static_cast<Func&&>(f));
    }

    template <class Grammar, class Func>
    constexpr auto parse(const char* begin, const char* end, Func&& f)
    {
        tokenizer<typename Grammar::token_spec> tok(begin, end);
        return parse<Grammar>(tok, static_cast<Func&&>(f));
    }
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_PARSER_HPP_INCLUDED
