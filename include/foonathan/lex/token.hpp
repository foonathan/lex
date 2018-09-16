// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_TOKEN_HPP_INCLUDED
#define FOONATHAN_LEX_TOKEN_HPP_INCLUDED

#include <foonathan/lex/spelling.hpp>
#include <foonathan/lex/token_kind.hpp>

namespace foonathan
{
namespace lex
{
    template <class TokenSpec>
    class tokenizer;

    /// A single token.
    ///
    /// \notes Tokens are lightweight views to the characters, they do not own them.
    template <class TokenSpec>
    class token
    {
    public:
        /// \effects Creates an invalid, partially-formed token that may not be used.
        constexpr token() noexcept : kind_(), size_(0), ptr_(nullptr) {}

        /// \returns The kind of token it is.
        constexpr token_kind<TokenSpec> kind() const noexcept
        {
            return kind_;
        }

        /// \returns Whether or not it is an error token.
        constexpr bool is_error() const noexcept
        {
            return !kind_;
        }

        /// \returns Whether or not it is the specified token.
        template <class Token>
        constexpr bool is(Token token = {}) const noexcept
        {
            return kind_.is(token);
        }

        /// \returns The spelling of the token.
        constexpr token_spelling spelling() const noexcept
        {
            return token_spelling(ptr_, size_);
        }

        /// \returns The offset of the token inside the character range of the tokenizer.
        constexpr std::size_t offset(const tokenizer<TokenSpec>& tokenizer) const noexcept
        {
            return static_cast<std::size_t>(ptr_ - tokenizer.begin_ptr());
        }

    private:
        explicit constexpr token(token_kind<TokenSpec> kind, const char* ptr,
                                 std::size_t size) noexcept
        : ptr_(ptr), size_(static_cast<std::uint32_t>(size)), kind_(kind)
        {}

        const char*           ptr_;
        std::uint32_t         size_;
        token_kind<TokenSpec> kind_;

        friend tokenizer<TokenSpec>;
    };
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_TOKEN_HPP_INCLUDED
