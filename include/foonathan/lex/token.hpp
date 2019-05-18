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

    template <class TokenSpec>
    class token
    {
    public:
        constexpr token() noexcept : ptr_(nullptr), size_(0), kind_() {}

        constexpr token_kind<TokenSpec> kind() const noexcept
        {
            return kind_;
        }

        explicit constexpr operator bool() const noexcept
        {
            return !!kind();
        }

        template <class Token>
        constexpr bool is(Token token = {}) const noexcept
        {
            return kind_.is(token);
        }

        template <template <typename> class Category>
        constexpr bool is_category() const noexcept
        {
            return kind_.template is_category<Category>();
        }

        constexpr const char* name() const noexcept
        {
            return kind_.name();
        }

        constexpr token_spelling spelling() const noexcept
        {
            return token_spelling(ptr_, size_);
        }

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

    template <class Token, class Payload = void>
    class static_token;

    template <class Token>
    class static_token<Token, void>
    {
        static_assert(is_token<Token>::value, "must be a token");

    public:
        template <class TokenSpec>
        explicit constexpr static_token(const token<TokenSpec>& token) : spelling_(token.spelling())
        {
            FOONATHAN_LEX_PRECONDITION(token.is(Token{}), "token kind must match");
        }

        constexpr operator Token() const noexcept
        {
            return Token{};
        }

        template <class TokenSpec>
        constexpr token_kind<TokenSpec> kind() const noexcept
        {
            return token_kind<TokenSpec>::template of<Token>();
        }

        explicit constexpr operator bool() const noexcept
        {
            return std::is_same<Token, error_token>::value;
        }

        template <class Other>
        constexpr bool is(Other = {}) const noexcept
        {
            return std::is_same<Token, Other>::value;
        }

        template <template <typename> class Category>
        constexpr bool is_category() const noexcept
        {
            return Category<Token>::value;
        }

        constexpr const char* name() const noexcept
        {
            return Token::name;
        }

        constexpr token_spelling spelling() const noexcept
        {
            return spelling_;
        }

        template <class TokenSpec>
        constexpr std::size_t offset(const tokenizer<TokenSpec>& tokenizer) const noexcept
        {
            return static_cast<std::size_t>(spelling_.data() - tokenizer.begin_ptr());
        }

    private:
        token_spelling spelling_;
    };

    template <class Token, class Payload>
    class static_token : static_token<Token, void>
    {
    public:
        template <class TokenSpec>
        explicit constexpr static_token(const token<TokenSpec>& token, Payload payload)
        : static_token<Token, void>(token), payload_(static_cast<Payload&&>(payload))
        {}

        constexpr Payload& value() & noexcept
        {
            return payload_;
        }
        constexpr const Payload& value() const& noexcept
        {
            return payload_;
        }
        constexpr Payload&& value() && noexcept
        {
            return static_cast<Payload&&>(payload_);
        }
        constexpr const Payload&& value() const&& noexcept
        {
            return static_cast<const Payload&&>(payload_);
        }

    private:
        Payload payload_;
    };
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_TOKEN_HPP_INCLUDED
