// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_TOKENIZER_HPP_INCLUDED
#define FOONATHAN_LEX_TOKENIZER_HPP_INCLUDED

#include <foonathan/lex/identifier.hpp>
#include <foonathan/lex/literal_token.hpp>
#include <foonathan/lex/rule_token.hpp>
#include <foonathan/lex/token.hpp>

namespace foonathan
{
namespace lex
{
    namespace detail
    {
        //=== try_match ===//
        template <class TokenSpec, class Root, class Rules>
        struct insert_rules
        {
            using type = Root;
        };

        template <class TokenSpec, class Root, class Head, class... Tail>
        struct insert_rules<TokenSpec, Root, type_list<Head, Tail...>>
        {
            using with_head = typename trie<TokenSpec>::template insert_rule<Root, Head>;
            using type      = typename insert_rules<TokenSpec, with_head, type_list<Tail...>>::type;
        };

        template <class TokenSpec>
        struct build_trie
        {
            // split the tokens
            using identifiers = keep_if<TokenSpec, is_identifier>;
            using keywords    = keep_if<TokenSpec, is_keyword>;
            using rule_tokens = keep_if<TokenSpec, is_non_identifier_rule_token>;
            using literals    = keep_if<TokenSpec, is_non_keyword_literal_token>;

            // start with the literal trie
            using trie0 = detail::literal_trie<TokenSpec, literals>;
            // insert all rule tokens
            using trie1 = typename insert_rules<TokenSpec, trie0, rule_tokens>::type;
            // insert the keyword identifier rule
            using keyword_identifier_matcher
                = detail::keyword_identifier_matcher<TokenSpec, identifiers, keywords>;
            using trie2 =
                typename detail::trie<TokenSpec>::template insert_rule<trie1,
                                                                       keyword_identifier_matcher>;
        };

        template <class TokenSpec>
        using token_spec_trie = typename build_trie<TokenSpec>::trie2;
    } // namespace detail

    /// Tokenizes a character range according the token specification.
    ///
    /// It will tokenize it by trying each of the specified tokens in an arbitrary order.
    /// The token that matches will be stored and the position advanced.
    /// If no token matched, it will store an error token and advance to the next character.
    /// If a token rule matched an error token, it will be transparently forwarded.
    ///
    /// It will only store one token in memory.
    /// Parsers requiring look ahead can be implemented by resetting the tokenizer to an earlier
    /// position, if necessary.
    template <class TokenSpec>
    class tokenizer
    {
        using trie = detail::token_spec_trie<TokenSpec>;

    public:
        //=== constructors ===//
        /// \effects Creates a tokenizer that will tokenize the range `[ptr, ptr + size)`.
        explicit constexpr tokenizer(const char* ptr, std::size_t size) noexcept
        : tokenizer(ptr, ptr + size)
        {}

        /// \effects Creates a tokenizer that will tokenize the range `[begin, end)`.
        explicit constexpr tokenizer(const char* begin, const char* end)
        : begin_(begin), ptr_(begin), end_(end), last_result_(match_result<TokenSpec>::unmatched())
        {
            reset(begin);
        }

        /// \effects Creates a tokenizer that will tokenize the given array *excluding* a null
        /// terminator.
        template <std::size_t N>
        explicit constexpr tokenizer(const char (&array)[N]) : tokenizer(array, array + N - 1)
        {}

        //=== tokenizer functions ===//
        /// \returns The current token.
        constexpr token<TokenSpec> peek() const noexcept
        {
            return token<TokenSpec>(last_result_.kind, ptr_, last_result_.bump);
        }

        /// \returns Whether or not EOF was reached.
        /// If this is `true`, `bump()` will have no effect anymore and `peek()` returns EOF.
        constexpr bool is_done() const noexcept
        {
            FOONATHAN_LEX_ASSERT(last_result_.bump != 0 || peek().is(eof{}));
            return last_result_.bump == 0;
        }

        /// Returns and advances the token.
        /// \effects Consumes the current token by calling `bump()`.
        /// \returns The current token, before it was consumed.
        constexpr token<TokenSpec> get() noexcept
        {
            auto result = peek();
            bump();
            return result;
        }

        /// \effects Consumes the current token, `peek()` will then return the next token.
        /// If `is_eof() == true` when calling the function it will replace the current token with
        /// EOF.
        constexpr void bump() noexcept
        {
            reset(ptr_ + last_result_.bump);
        }

        /// \effects Resets the tokenizer to the specified position and parses that token
        /// immediately.
        constexpr void reset(const char* position) noexcept
        {
            FOONATHAN_LEX_PRECONDITION(begin_ <= position && position <= end_,
                                       "position out of range");
            ptr_         = position;
            last_result_ = trie::try_match(ptr_, end_);
        }

        //=== getters ===//
        /// \returns The beginning of the character range.
        constexpr const char* begin_ptr() const noexcept
        {
            return begin_;
        }

        /// \returns The current position in the character range.
        /// `peek()` returns the token starting at that position.
        constexpr const char* current_ptr() const noexcept
        {
            FOONATHAN_LEX_ASSERT(peek().spelling().data() == ptr_);
            return ptr_;
        }

        /// \returns One past the end of the character range.
        constexpr const char* end_ptr() const noexcept
        {
            return end_;
        }

    private:
        const char* begin_{};
        const char* ptr_{};
        const char* end_{};

        match_result<TokenSpec> last_result_;
    };
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_TOKENIZER_HPP_INCLUDED
