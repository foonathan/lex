// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
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
            //=== node lookup ===//
            // finds a node for the given character
            template <char C>
            struct node_finder
            {
                template <typename Node>
                struct predicate : std::integral_constant<bool, Node::character == C>
                {};
            };

            // error node in case none is found
            struct error_node
            {
                static constexpr auto is_terminal = false;
                using children                    = type_list<>;
            };

            // the node that matches the given character
            template <char C, class Nodes>
            using matching_node =
                typename keep_if<Nodes,
                                 node_finder<C>::template predicate>::template type_or<error_node>;

            // inserts the new node into the list of node, replacing any previous ones if necessary
            template <class NewNode, class Nodes>
            using insert_node
                = concat<remove_if<Nodes, node_finder<NewNode::character>::template predicate>,
                         NewNode>;

            template <class Rule, class Children>
            struct insert_rule_into_children_impl;
            template <class Rule, class... Children>
            struct insert_rule_into_children_impl<Rule, type_list<Children...>>
            {
                using type = type_list<typename Children::template insert_rule<Rule>...>;
            };

            template <class Rule, class Children>
            using insert_rule_into_children =
                typename insert_rule_into_children_impl<Rule, Children>::type;

            //=== nodes ===//
            // tries to match all children
            template <class... Children>
            static constexpr auto try_match_children(type_list<Children...>,
                                                     std::size_t length_so_far, const char* str,
                                                     const char* end) noexcept
            {
                // need to check for EOF now
                if (str == end)
                    return match_result<TokenSpec>::eof();

                auto result  = match_result<TokenSpec>::unmatched();
                bool dummy[] = {(result.is_unmatched() && *str == Children::character
                                 && (result = Children::match(length_so_far, str, end), true))...,
                                true};
                (void)dummy;
                return result;
            }
            // optimizations for 0 and 1
            static constexpr auto try_match_children(type_list<>, std::size_t, const char* str,
                                                     const char* end) noexcept
            {
                if (str == end)
                    return match_result<TokenSpec>::eof();
                else
                    return match_result<TokenSpec>::unmatched();
            }
            template <class Child>
            static constexpr auto try_match_children(type_list<Child>, std::size_t length_so_far,
                                                     const char* str, const char* end) noexcept
            {
                if (str == end)
                    return match_result<TokenSpec>::eof();
                else if (*str == Child::character)
                    return Child::match(length_so_far, str, end);
                else
                    return match_result<TokenSpec>::unmatched();
            }

            // tries to match all rules
            template <class... Rules>
            static constexpr auto try_match_rules(type_list<Rules...>, std::size_t length_so_far,
                                                  const char* str, const char* end) noexcept
            {
                // no need to check for EOF, only called after literal tokens

                str -= length_so_far;

                auto result = match_result<TokenSpec>::unmatched();
                bool dummy[]
                    = {(result.is_unmatched() && (result = Rules::try_match(str, end), true))...,
                       true};
                (void)dummy;
                return result;
            }
            // optimizations for 0 and 1
            static constexpr auto try_match_rules(type_list<>, std::size_t, const char*,
                                                  const char*) noexcept
            {
                return match_result<TokenSpec>::unmatched();
            }
            template <class Rule>
            static constexpr auto try_match_rules(type_list<Rule>, std::size_t length_so_far,
                                                  const char* str, const char* end) noexcept
            {
                return Rule::try_match(str - length_so_far, end);
            }

            // a non-terminal node matching the given character
            template <char C, class ChildNodes>
            struct non_terminal_node
            {
                static constexpr auto is_terminal = false;
                using children                    = ChildNodes;
                static constexpr auto character   = C;

                template <class Child>
                using insert = non_terminal_node<C, insert_node<Child, ChildNodes>>;

                template <class Rule>
                using insert_rule
                    // just insert the rule into all children
                    = non_terminal_node<C, insert_rule_into_children<Rule, ChildNodes>>;

                static constexpr auto match(std::size_t length_so_far, const char* str,
                                            const char* end) noexcept
                {
                    return try_match_children(ChildNodes{}, length_so_far + 1, str + 1, end);
                }
            };

            // a terminal node terminating token Id with the given character
            template <char C, id_type<TokenSpec> Id, class ChildNodes, class... Rules>
            struct terminal_node
            {
                static constexpr auto is_terminal = true;
                using children                    = ChildNodes;
                static constexpr auto character   = C;

                template <class Child>
                using insert = terminal_node<C, Id, insert_node<Child, ChildNodes>, Rules...>;

                template <class Rule>
                using insert_rule = std::conditional_t<
                    Rule::is_conflicting_literal(token_kind<TokenSpec>::from_id(Id)),
                    // if it is conflicting: inserto into *this and children
                    terminal_node<C, Id, insert_rule_into_children<Rule, ChildNodes>, Rules...,
                                  Rule>,
                    // otherwise just into children
                    terminal_node<C, Id, insert_rule_into_children<Rule, ChildNodes>, Rules...>>;

                static constexpr auto match(std::size_t length_so_far, const char* str,
                                            const char* end) noexcept
                {
                    ++length_so_far;
                    ++str;

                    // check for a longer match
                    auto child_result = try_match_children(ChildNodes{}, length_so_far, str, end);
                    if (child_result.is_success())
                        // found a longer match
                        return child_result;

                    // check the conflicting rules
                    // if a longer token match happened, longer token was also conflicting
                    auto rule_result
                        = try_match_rules(type_list<Rules...>{}, length_so_far, str, end);
                    if (rule_result.is_matched())
                        // rule matched something
                        return rule_result;

                    // only then match the token
                    return match_result<TokenSpec>::success(token_kind<TokenSpec>::from_id(Id),
                                                            length_so_far);
                }
            };

            // the root node, with additional rules
            template <class ChildNodes, class... Rules>
            struct root_node
            {
                static constexpr auto is_terminal = false;
                using children                    = ChildNodes;

                template <class Child>
                using insert = root_node<insert_node<Child, ChildNodes>, Rules...>;

                template <class Rule>
                using insert_rule
                    = root_node<insert_rule_into_children<Rule, ChildNodes>, Rules..., Rule>;

                static constexpr auto try_match(const char* str, const char* end) noexcept
                {
                    // match all literals
                    auto child_result = try_match_children(ChildNodes{}, 0, str, end);
                    if (child_result.is_matched())
                        return child_result;

                    // now match all rules
                    auto rule_result = try_match_rules(type_list<Rules...>{}, 0, str, end);
                    if (rule_result.is_matched())
                        return rule_result;

                    // nothing matched, error
                    return match_result<TokenSpec>::error(1);
                }

                static constexpr auto try_match(const char* str, std::size_t size) noexcept
                {
                    return try_match(str, str + size);
                }
            };

            //=== trie construction ===//
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
            // an empty trie
            using empty = root_node<type_list<>>;

            // inserts a literal token into the trie
            template <class Root, id_type<TokenSpec> Id, char... Chars>
            using insert_literal = typename insert_literal_impl<Root, Id, Chars...>::type;

            // inserts a literal token where `String = StringTemplate<Chars...>`
            template <class Root, id_type<TokenSpec> Id, typename String>
            using insert_literal_str = typename insert_literal_str_impl<Root, Id, String>::type;

            // inserts a rule
            template <class Root, class Rule>
            using insert_rule = typename Root::template insert_rule<Rule>;
        };
    } // namespace detail
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_DETAIL_TRIE_HPP_INCLUDED
