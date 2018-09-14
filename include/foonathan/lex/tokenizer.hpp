// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_TOKENIZER_HPP_INCLUDED
#define FOONATHAN_LEX_TOKENIZER_HPP_INCLUDED

#include <foonathan/lex/rule_token.hpp>
#include <foonathan/lex/token.hpp>

namespace foonathan
{
    namespace lex
    {
        namespace detail
        {
            template <class TokenSpec, typename... Tokens>
            struct try_parse_impl;

            template <class TokenSpec>
            struct try_parse_impl<TokenSpec>
            {
                static constexpr parse_rule_result<TokenSpec> parse(const char*,
                                                                    const char*) noexcept
                {
                    // nothing matched, create an error
                    return parse_rule_result<TokenSpec>(1);
                }
            };

            template <class TokenSpec, typename Head, typename... Tail>
            struct try_parse_impl<TokenSpec, Head, Tail...>
            {
                static constexpr parse_rule_result<TokenSpec> parse(const char* str,
                                                                    const char* end) noexcept
                {
                    static_assert(std::is_base_of<rule_token<TokenSpec>, Head>::value, "");
                    auto result = Head::try_match(str, end);
                    if (result.is_matched())
                        return result;
                    else
                        return try_parse_impl<TokenSpec, Tail...>::parse(str, end);
                }
            };

            template <class TokenSpec, typename... Types>
            constexpr parse_rule_result<TokenSpec> try_parse(type_list<Types...>, const char* str,
                                                             const char* end) noexcept
            {
                return try_parse_impl<TokenSpec, Types...>::parse(str, end);
            }

            template <class TokenSpec>
            constexpr parse_rule_result<TokenSpec> try_parse(const char* str,
                                                             const char* end) noexcept
            {
                return try_parse<TokenSpec>(TokenSpec{}, str, end);
            }
        } // namespace detail

        template <class TokenSpec>
        class tokenizer
        {
        public:
            explicit constexpr tokenizer(const char* ptr, std::size_t size)
            : begin_(ptr), ptr_(ptr), end_(ptr + size)
            {
                bump();
            }

            /// \returns Whether or not EOF was reached.
            constexpr bool is_eof() const noexcept
            {
                return ptr_ == end_;
            }

            /// \returns The current token.
            constexpr token<TokenSpec> peek() const noexcept
            {
                return cur_;
            }

            /// \effects Consumes the current token by calling `bump()`.
            /// \returns The current token, before it was consumed.
            constexpr token<TokenSpec> get() noexcept
            {
                auto result = peek();
                bump();
                return result;
            }

            /// \effects Consumes the current token.
            /// If `is_eof() == true`, does nothing.
            constexpr void bump() noexcept
            {
                if (ptr_ == end_)
                    return;

                auto result = detail::try_parse<TokenSpec>(ptr_, end_);
                // TODO: assert result.is_matched() && range of ptr_
                cur_ = token<TokenSpec>(result.kind, token_spelling(ptr_, result.bump),
                                        static_cast<std::size_t>(ptr_ - begin_));
                ptr_ += result.bump;
            }

        private:
            const char* begin_;
            const char* ptr_;
            const char* end_;

            token<TokenSpec> cur_;
        };
    } // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_TOKENIZER_HPP_INCLUDED
