// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
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
        constexpr token() noexcept : ptr_(nullptr), size_(0), kind_() {}

        /// \returns The kind of token it is.
        constexpr token_kind<TokenSpec> kind() const noexcept
        {
            return kind_;
        }

        /// \returns `!!kind()`.
        explicit constexpr operator bool() const noexcept
        {
            return !!kind();
        }

        /// \returns `kind().is(token)`.
        template <class Token>
        constexpr bool is(Token token = {}) const noexcept
        {
            return kind_.is(token);
        }

        /// \returns `kind().is_category<Category>()`.
        template <template <typename> class Category>
        constexpr bool is_category() const noexcept
        {
            return kind_.template is_category<Category>();
        }

        /// \returns `kind().name()`.
        constexpr const char* name() const noexcept
        {
            return kind_.name();
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
        : ptr_(ptr), size_(size), kind_(kind)
        {}

        const char*           ptr_;
        std::size_t           size_;
        token_kind<TokenSpec> kind_;

        friend tokenizer<TokenSpec>;
    };
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_TOKEN_HPP_INCLUDED
