// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_DETAIL_TRIE_HPP_INCLUDED
#define FOONATHAN_LEX_DETAIL_TRIE_HPP_INCLUDED

#include <foonathan/lex/detail/select_integer.hpp>
#include <foonathan/lex/detail/type_list.hpp>
#include <foonathan/lex/match_result.hpp>

namespace foonathan
{
namespace lex
{
    namespace detail
    {
        template <class TokenSpec>
        class trie
        {
            template <char C>
            struct node_char_finder
            {
                template <typename Node>
                struct predicate : std::integral_constant<bool, Node::character == C>
                {};
            };

            template <class Target, typename = void>
            struct node_finder
            {
                template <typename Node>
                struct predicate : std::false_type
                {};
            };

            template <class Target>
            struct node_finder<Target, decltype(void(Target::character))>
            : node_char_finder<Target::character>
            {};

            struct error_node
            {
                static constexpr auto is_terminal = false;
                using children                    = type_list<>;
            };

            template <char C, class Nodes>
            using matching_node = typename keep_if<
                Nodes, node_char_finder<C>::template predicate>::template type_or<error_node>;

            template <class NewNode, class Nodes>
            using insert_node
                = concat<remove_if<Nodes, node_finder<NewNode>::template predicate>, NewNode>;

            template <class... Children>
            static constexpr auto try_match_children(type_list<Children...>,
                                                     std::size_t length_so_far, const char* str,
                                                     const char* end) noexcept
            {
                match_result<TokenSpec> result;
                bool                    dummy[]
                    = {(!result.is_matched()
                        && (result = Children::try_match(length_so_far, str, end), true))...};
                (void)dummy;
                return result;
            }

            template <char C, class ChildNodes>
            struct non_terminal_node
            {
                static constexpr auto is_terminal = false;
                using children                    = ChildNodes;
                static constexpr auto character   = C;

                template <class Child>
                using insert = non_terminal_node<C, insert_node<Child, ChildNodes>>;

                static constexpr auto try_match(std::size_t length_so_far, const char* str,
                                                const char* end) noexcept
                {
                    if (str == end)
                        return match_result<TokenSpec>(token_kind<TokenSpec>(eof{}), 0);
                    else if (*str != C)
                        return match_result<TokenSpec>();
                    else
                        return try_match_children(ChildNodes{}, length_so_far + 1, str + 1, end);
                }
            };

            template <char C, id_type<TokenSpec> Id, class ChildNodes>
            struct terminal_node
            {
                static constexpr auto is_terminal = true;
                using children                    = ChildNodes;
                static constexpr auto character   = C;

                template <class Child>
                using insert = terminal_node<C, Id, insert_node<Child, ChildNodes>>;

                static constexpr auto try_match(std::size_t length_so_far, const char* str,
                                                const char* end) noexcept
                {
                    if (str == end)
                        return match_result<TokenSpec>(token_kind<TokenSpec>(eof{}), 0);
                    else if (*str != C)
                        return match_result<TokenSpec>();
                    else
                    {
                        auto child_result
                            = try_match_children(ChildNodes{}, length_so_far + 1, str + 1, end);
                        if (child_result.is_matched())
                            return child_result;
                        else
                            return match_result<TokenSpec>(token_kind<TokenSpec>::from_id(Id),
                                                           length_so_far + 1);
                    }
                }
            };

            template <class ChildNodes, class... Matchers>
            struct root_node
            {
                static constexpr auto is_terminal = false;
                using children                    = ChildNodes;

                template <class Child>
                using insert = root_node<insert_node<Child, ChildNodes>, Matchers...>;

                template <class Matcher>
                using insert_matcher = root_node<ChildNodes, Matchers..., Matcher>;

                static constexpr auto lookup_prefix(const char* str, const char* end) noexcept
                {
                    auto child_result = try_match_children(ChildNodes{}, 0, str, end);
                    if (child_result.is_matched())
                        return child_result;
                    else if (str == end)
                        return match_result<TokenSpec>(token_kind<TokenSpec>(eof{}), 0);

                    match_result<TokenSpec> result;
                    bool                    dummy[] = {(!result.is_matched()
                                     && (result = Matchers::try_match(str, end), true))...};
                    (void)dummy;

                    if (result.is_matched())
                        return result;
                    else
                        return match_result<TokenSpec>(1);
                }

                static constexpr auto lookup_prefix(const char* str, std::size_t size) noexcept
                {
                    return lookup_prefix(str, str + size);
                }
            };

            template <class CurNode, id_type<TokenSpec> Id, char... Chars>
            struct insert_literal_impl;

            template <class CurNode, id_type<TokenSpec> Id, char C>
            struct insert_literal_impl<CurNode, Id, C>
            {
                // the target is the C-child of the current node
                using target_node = matching_node<C, typename CurNode::children>;
                static_assert(!target_node::is_terminal, "duplicate string insert into trie");

                // need to turn the target into a terminal node with the same character and children
                using new_node = terminal_node<C, Id, typename target_node::children>;

                using type = typename CurNode::template insert<new_node>;
            };

            template <class CurNode, id_type<TokenSpec> Id, char Head, char Second, char... Rest>
            struct insert_literal_impl<CurNode, Id, Head, Second, Rest...>
            {
                // the target is the Head-child of the current node
                using target_node = matching_node<Head, typename CurNode::children>;
                using no_target   = std::is_same<target_node, error_node>;

                // if the target doesn't exist we need to create a new non-terminal node
                using actual_target
                    = std::conditional_t<no_target::value, non_terminal_node<Head, type_list<>>,
                                         target_node>;

                // insert the children into the actual target
                using inserted =
                    typename insert_literal_impl<actual_target, Id, Second, Rest...>::type;

                // insert the target with children into the current node
                using type = typename CurNode::template insert<inserted>;
            };

            template <class CurNode, id_type<TokenSpec> Id, typename String>
            struct insert_literal_str_impl;

            template <class CurNode, id_type<TokenSpec> Id, template <char...> class String,
                      char... Char>
            struct insert_literal_str_impl<CurNode, Id, String<Char...>>
            {
                using type = typename insert_literal_impl<CurNode, Id, Char...>::type;
            };

        public:
            using empty = root_node<type_list<>>;

            template <class Root, id_type<TokenSpec> Id, char... Chars>
            using insert_literal = typename insert_literal_impl<Root, Id, Chars...>::type;

            template <class Root, id_type<TokenSpec> Id, typename String>
            using insert_literal_str = typename insert_literal_str_impl<Root, Id, String>::type;

            template <class Root, class Matcher>
            using insert_matcher = typename Root::template insert_matcher<Matcher>;
        };
    } // namespace detail
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_DETAIL_TRIE_HPP_INCLUDED
