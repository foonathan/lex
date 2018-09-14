// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_TOKENIZER_HPP_INCLUDED
#define FOONATHAN_LEX_TOKENIZER_HPP_INCLUDED

#include <foonathan/lex/detail/trie.hpp>
#include <foonathan/lex/literal_token.hpp>
#include <foonathan/lex/rule_token.hpp>
#include <foonathan/lex/token.hpp>

namespace foonathan
{
    namespace lex
    {
        namespace detail
        {
            //=== literal parsing ===//
            template <class LiteralTokens>
            struct trie_nodes_needed;

            template <>
            struct trie_nodes_needed<type_list<>> : std::integral_constant<std::size_t, 0>
            {
            };

            template <typename Head, typename... Tail>
            struct trie_nodes_needed<type_list<Head, Tail...>>
            : std::integral_constant<
                  std::size_t,
                  // for each literal one node for each character in the string (excluding null) in the worst case
                  sizeof(Head::value) - 1 + trie_nodes_needed<type_list<Tail...>>::value>
            {
            };

            template <class TokenSpec, class LiteralTokens>
            using literal_trie =
                trie<token_kind<TokenSpec>, trie_nodes_needed<LiteralTokens>::value>;

            template <class Trie, class LiteralTokens>
            struct build_trie_impl;

            template <class Trie>
            struct build_trie_impl<Trie, type_list<>>
            {
                static constexpr void insert(Trie&) noexcept {}
            };

            template <class Trie, typename Head, typename... Tail>
            struct build_trie_impl<Trie, type_list<Head, Tail...>>
            {
                static constexpr void insert(Trie& trie) noexcept
                {
                    trie.insert(Head::value, typename Trie::user_data(Head{}));
                    build_trie_impl<Trie, type_list<Tail...>>::insert(trie);
                }
            };

            template <class TokenSpec, class LiteralTokens>
            constexpr literal_trie<TokenSpec, LiteralTokens> build_trie() noexcept
            {
                literal_trie<TokenSpec, LiteralTokens> result;
                build_trie_impl<decltype(result), LiteralTokens>::insert(result);
                return result;
            }

            //=== rule parsing ===//
            template <class TokenSpec, typename... Tokens>
            struct rule_match_impl;

            template <class TokenSpec>
            struct rule_match_impl<TokenSpec>
            {
                static constexpr match_result<TokenSpec> parse(const char*, const char*) noexcept
                {
                    // nothing matched, create an error
                    return match_result<TokenSpec>(1);
                }
            };

            template <class TokenSpec, typename Head, typename... Tail>
            struct rule_match_impl<TokenSpec, Head, Tail...>
            {
                static constexpr match_result<TokenSpec> parse(const char* str,
                                                               const char* end) noexcept
                {
                    static_assert(is_rule_token<Head>::value, "");
                    auto result = Head::try_match(str, end);
                    if (result.is_matched())
                        return result;
                    else
                        return rule_match_impl<TokenSpec, Tail...>::parse(str, end);
                }
            };

            template <class TokenSpec, typename... Types>
            constexpr match_result<TokenSpec> rule_match(type_list<Types...>, const char* str,
                                                         const char* end) noexcept
            {
                return rule_match_impl<TokenSpec, Types...>::parse(str, end);
            }

            //=== interface ===//
            template <class TokenSpec>
            constexpr match_result<TokenSpec> try_match(const char* str, const char* end) noexcept
            {
                using literals      = keep_if<TokenSpec, is_literal_token>;
                constexpr auto trie = build_trie<TokenSpec, literals>();
                if (auto result = trie.lookup_prefix(str, end))
                    return match_result<TokenSpec>(result.data, result.prefix_length);
                else
                    return rule_match<TokenSpec>(keep_if<TokenSpec, is_rule_token>{}, str, end);
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

                auto result = detail::try_match<TokenSpec>(ptr_, end_);
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
