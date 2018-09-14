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
        class token
        {
        public:
            constexpr token() noexcept : kind_(), size_(0), str_(nullptr), offset_(0) {}

            explicit constexpr token(token_kind<TokenSpec> kind, token_spelling spelling,
                                     std::size_t offset) noexcept
            : kind_(kind),
              size_(static_cast<std::uint32_t>(spelling.size())),
              str_(spelling.data()),
              offset_(static_cast<std::uint64_t>(offset))
            {
            }

            constexpr token_kind<TokenSpec> kind() const noexcept
            {
                return kind_;
            }

            template <class Token>
            constexpr bool is(Token token = {}) const noexcept
            {
                return kind_.is(token);
            }

            constexpr bool is_error() const noexcept
            {
                return !kind_;
            }

            constexpr token_spelling spelling() const noexcept
            {
                return token_spelling(str_, size_);
            }

            constexpr std::size_t offset() const noexcept
            {
                return static_cast<std::size_t>(offset_);
            }

        private:
            token_kind<TokenSpec> kind_;
            std::uint32_t         size_;
            const char*           str_;
            std::uint64_t         offset_;
        };
    } // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_TOKEN_HPP_INCLUDED
