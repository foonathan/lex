// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_DETAIL_TRIE_HPP_INCLUDED
#define FOONATHAN_LEX_DETAIL_TRIE_HPP_INCLUDED

#include <foonathan/lex/detail/constexpr_vector.hpp>
#include <foonathan/lex/detail/select_integer.hpp>
#include <foonathan/lex/detail/type_list.hpp>

namespace foonathan
{
namespace lex
{
    namespace detail
    {
        template <typename UserData>
        class trie
        {
            struct lookup_result
            {
                UserData    data;
                std::size_t length;

                explicit constexpr operator bool() const noexcept
                {
                    return length > 0;
                }
            };

            template <char C>
            struct node_finder
            {
                template <typename Node>
                struct predicate : std::integral_constant<bool, Node::character == C>
                {};
            };

            struct error_node;

            template <char C, class Nodes>
            using matching_node =
                typename keep_if<Nodes,
                                 node_finder<C>::template predicate>::template type_or<error_node>;

            template <class NewNode, class Nodes>
            using insert_node
                = concat<remove_if<Nodes, node_finder<NewNode::character>::template predicate>,
                         NewNode>;

            struct error_node
            {
                static constexpr auto is_terminal = false;
                using children                    = type_list<>;

                static constexpr lookup_result lookup_prefix(std::size_t, const char*,
                                                             const char*) noexcept
                {
                    return lookup_result{{}, 0};
                }
            };

            template <class ChildNodes>
            static constexpr lookup_result lookup_prefix_impl(std::size_t length_so_far,
                                                              const char* str,
                                                              const char* end) noexcept
            {
#define FOONATHAN_LEX_DETAIL_CASE_0(Value)                                                         \
case Value:                                                                                        \
    return matching_node<Value, ChildNodes>::lookup_prefix(length_so_far + 1, str + 1, end);
#define FOONATHAN_LEX_DETAIL_CASE_1(Value)                                                         \
    FOONATHAN_LEX_DETAIL_CASE_0(Value)                                                             \
    FOONATHAN_LEX_DETAIL_CASE_0(Value + 1)
#define FOONATHAN_LEX_DETAIL_CASE_2(Value)                                                         \
    FOONATHAN_LEX_DETAIL_CASE_1(Value)                                                             \
    FOONATHAN_LEX_DETAIL_CASE_1(Value + 2)
#define FOONATHAN_LEX_DETAIL_CASE_3(Value)                                                         \
    FOONATHAN_LEX_DETAIL_CASE_2(Value)                                                             \
    FOONATHAN_LEX_DETAIL_CASE_2(Value + 4)
#define FOONATHAN_LEX_DETAIL_CASE_4(Value)                                                         \
    FOONATHAN_LEX_DETAIL_CASE_3(Value)                                                             \
    FOONATHAN_LEX_DETAIL_CASE_3(Value + 8)
#define FOONATHAN_LEX_DETAIL_CASE_5(Value)                                                         \
    FOONATHAN_LEX_DETAIL_CASE_4(Value)                                                             \
    FOONATHAN_LEX_DETAIL_CASE_4(Value + 16)
#define FOONATHAN_LEX_DETAIL_CASE_6(Value)                                                         \
    FOONATHAN_LEX_DETAIL_CASE_5(Value)                                                             \
    FOONATHAN_LEX_DETAIL_CASE_5(Value + 32)
#define FOONATHAN_LEX_DETAIL_CASE_7(Value)                                                         \
    FOONATHAN_LEX_DETAIL_CASE_6(Value)                                                             \
    FOONATHAN_LEX_DETAIL_CASE_6(Value + 64)

                if (str != end)
                {
                    switch (*str)
                    {
                        FOONATHAN_LEX_DETAIL_CASE_7(0)

                    default:
                        break;
                    }
                }

                return lookup_result{{}, 0};
            }

            template <char C, class ChildNodes>
            struct non_terminal_node
            {
                static constexpr auto is_terminal = false;
                using children                    = ChildNodes;
                static constexpr auto character   = C;

                template <class Child>
                using insert = non_terminal_node<C, insert_node<Child, ChildNodes>>;

                static constexpr lookup_result lookup_prefix(std::size_t length_so_far,
                                                             const char* str,
                                                             const char* end) noexcept
                {
                    return trie::lookup_prefix_impl<ChildNodes>(length_so_far, str, end);
                }
            };

            template <char C, UserData data, class ChildNodes>
            struct terminal_node
            {
                static constexpr auto is_terminal = true;
                using children                    = ChildNodes;
                static constexpr auto character   = C;

                template <class Child>
                using insert = terminal_node<C, data, insert_node<Child, ChildNodes>>;

                static constexpr lookup_result lookup_prefix(std::size_t length_so_far,
                                                             const char* str,
                                                             const char* end) noexcept
                {
                    auto longer_result
                        = trie::lookup_prefix_impl<ChildNodes>(length_so_far, str, end);
                    if (longer_result.length > 0)
                        return longer_result;
                    else
                        return lookup_result{data, length_so_far};
                }
            };

            template <class ChildNodes>
            struct root_node
            {
                static constexpr auto is_terminal = false;
                using children                    = ChildNodes;

                template <class Child>
                using insert = root_node<insert_node<Child, ChildNodes>>;

                static constexpr lookup_result lookup_prefix(const char* str,
                                                             const char* end) noexcept
                {
                    return trie::lookup_prefix_impl<ChildNodes>(0, str, end);
                }

                static constexpr lookup_result lookup_prefix(const char* str,
                                                             std::size_t size) noexcept
                {
                    return trie::lookup_prefix_impl<ChildNodes>(0, str, str + size);
                }
            };

            template <class CurNode, UserData Data, char... Chars>
            struct insert_impl;

            template <class CurNode, UserData Data, char C>
            struct insert_impl<CurNode, Data, C>
            {
                // the target is the C-child of the current node
                using target_node = matching_node<C, typename CurNode::children>;
                static_assert(!target_node::is_terminal, "duplicate string insert into trie");

                // need to turn the target into a terminal node with the same character and children
                using new_node = terminal_node<C, Data, typename target_node::children>;

                using type = typename CurNode::template insert<new_node>;
            };

            template <class CurNode, UserData Data, char Head, char Second, char... Rest>
            struct insert_impl<CurNode, Data, Head, Second, Rest...>
            {
                // the target is the Head-child of the current node
                using target_node = matching_node<Head, typename CurNode::children>;
                using no_target   = std::is_same<target_node, error_node>;

                // if the target doesn't exist we need to create a new non-terminal node
                using actual_target
                    = std::conditional_t<no_target::value, non_terminal_node<Head, type_list<>>,
                                         target_node>;

                // insert the children into the actual target
                using inserted = typename insert_impl<actual_target, Data, Second, Rest...>::type;

                // insert the target with children into the current node
                using type = typename CurNode::template insert<inserted>;
            };

            template <class CurNode, UserData Data, typename String>
            struct insert_string_impl;

            template <class CurNode, UserData Data, template <char...> class String, char... Char>
            struct insert_string_impl<CurNode, Data, String<Char...>>
            {
                using type = typename insert_impl<CurNode, Data, Char...>::type;
            };

        public:
            using empty = root_node<type_list<>>;

            template <class Root, UserData Data, char... Chars>
            using insert = typename insert_impl<Root, Data, Chars...>::type;

            template <class Root, UserData Data, typename String>
            using insert_string = typename insert_string_impl<Root, Data, String>::type;
        };
    } // namespace detail
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_DETAIL_TRIE_HPP_INCLUDED
