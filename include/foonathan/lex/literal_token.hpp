// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_LITERAL_TOKEN_HPP_INCLUDED
#define FOONATHAN_LEX_LITERAL_TOKEN_HPP_INCLUDED

#include <foonathan/lex/detail/string.hpp>
#include <foonathan/lex/detail/trie.hpp>
#include <foonathan/lex/match_result.hpp>
#include <foonathan/lex/token_kind.hpp>

namespace foonathan
{
namespace lex
{
    /// A literal token.
    ///
    /// It matches the given sequence of characters.
    /// If there are multiple literal tokens with a common prefix the longest matching will be
    /// selected.
    template <char... Literal>
    struct literal_token
    {
        static constexpr const char value[] = {Literal..., '\0'};
        static_assert(value[sizeof...(Literal) - 1] != '\0', "literal must not be null-terminated");

        static constexpr const char* name = value;
    };

    template <char... Literal>
    constexpr const char literal_token<Literal...>::value[];

    /// Expands to `literal_token<String[0], String[1], ...>`.
    /// It ignores all null characters.
#define FOONATHAN_LEX_LITERAL(String)                                                              \
    FOONATHAN_LEX_DETAIL_STRING(foonathan::lex::literal_token, String)

    namespace detail
    {
        template <template <char...> class Kind, class Token>
        struct is_literal_token_impl
        {
            template <char... Literal>
            static std::true_type  test(const Kind<Literal...>&);
            static std::false_type test(...);

            using value = decltype(test(std::declval<Token>()));
        };
    } // namespace detail

    /// Whether or not the token is a literal token.
    template <class Token>
    struct is_literal_token : detail::is_literal_token_impl<literal_token, Token>::value
    {};

    namespace detail
    {
        //=== literal_trie type ===//
        template <class LiteralTokens>
        struct trie_nodes_needed;

        template <>
        struct trie_nodes_needed<type_list<>> : std::integral_constant<std::size_t, 0>
        {};

        template <typename Head, typename... Tail>
        struct trie_nodes_needed<type_list<Head, Tail...>>
        : std::integral_constant<std::size_t,
                                 // for each literal one node for each character in the string
                                 // (excluding null) in the worst case
                                 sizeof(Head::value) - 1
                                     + trie_nodes_needed<type_list<Tail...>>::value>
        {};

        template <class TokenSpec, class LiteralTokens>
        using literal_trie = trie<token_kind<TokenSpec>, trie_nodes_needed<LiteralTokens>::value>;

        //=== literal trie building ===//
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

        //=== literal_matcher ===//
        template <class TokenSpec, class LiteralTokens>
        struct literal_matcher
        {
            static constexpr auto trie = build_trie<TokenSpec, LiteralTokens>();

            static constexpr match_result<TokenSpec> try_match(const char* str,
                                                               const char* end) noexcept
            {
                if (auto result = trie.lookup_prefix(str, end))
                    return match_result<TokenSpec>(result.data, result.prefix_length);
                else
                    return match_result<TokenSpec>();
            }
        };
    } // namespace detail
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_LITERAL_TOKEN_HPP_INCLUDED
