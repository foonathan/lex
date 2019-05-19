// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_DETAIL_TRIE_HPP_INCLUDED
#define FOONATHAN_LEX_DETAIL_TRIE_HPP_INCLUDED

#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/list.hpp>
#include <boost/mp11/utility.hpp>

#include <foonathan/lex/detail/select_integer.hpp>
#include <foonathan/lex/match_result.hpp>

namespace foonathan
{
namespace lex
{
    namespace detail
    {
        namespace mp = boost::mp11;

        //=== apply_q_char ==//
        template <template <char...> class Func, class List>
        struct apply_char_impl;
        template <template <char...> class Func, template <char...> class Templ, char... C>
        struct apply_char_impl<Func, Templ<C...>>
        {
            using type = Func<C...>;
        };

        template <class Q, class List>
        using apply_q_char = typename apply_char_impl<Q::template fn, List>::type;

        //=== matcher functions ===//
        // tries to match all children
        template <class TokenSpec, class... Children>
        struct children_matcher
        {
            static constexpr match_result<TokenSpec> try_match(std::size_t length_so_far,
                                                               const char* str,
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
        };
        template <class TokenSpec, class Child>
        struct children_matcher<TokenSpec, Child>
        {
            static constexpr match_result<TokenSpec> try_match(std::size_t length_so_far,
                                                               const char* str,
                                                               const char* end) noexcept
            {
                if (str == end)
                    return match_result<TokenSpec>::eof();
                else if (*str == Child::character)
                    return Child::match(length_so_far, str, end);
                else
                    return match_result<TokenSpec>::unmatched();
            }
        };
        template <class TokenSpec>
        struct children_matcher<TokenSpec>
        {
            static constexpr match_result<TokenSpec> try_match(std::size_t, const char* str,
                                                               const char* end) noexcept
            {
                if (str == end)
                    return match_result<TokenSpec>::eof();
                else
                    return match_result<TokenSpec>::unmatched();
            }
        };

        // tries to match all rules
        template <class TokenSpec, class... Rules>
        struct rule_matcher
        {
            static constexpr match_result<TokenSpec> try_match(std::size_t length_so_far,
                                                               const char* str,
                                                               const char* end) noexcept
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
        };
        template <class TokenSpec, class Rule>
        struct rule_matcher<TokenSpec, Rule>
        {
            static constexpr match_result<TokenSpec> try_match(std::size_t length_so_far,
                                                               const char* str,
                                                               const char* end) noexcept
            {
                return Rule::try_match(str - length_so_far, end);
            }
        };
        template <class TokenSpec>
        struct rule_matcher<TokenSpec>
        {
            static constexpr match_result<TokenSpec> try_match(std::size_t, const char*,
                                                               const char*) noexcept
            {
                return match_result<TokenSpec>::unmatched();
            }
        };

        //=== trie node lookup and manipulation ===//
        // finds a node for the given character
        template <char C>
        struct node_finder
        {
            template <typename Node>
            using fn = std::integral_constant<bool, Node::character == C>;
        };

        struct no_matching_node
        {
            static constexpr bool is_terminal = false;
            using children                    = mp::mp_list<>;
        };

        // the node that matches the given character, or no_matching_node
        template <char C, class Nodes>
        using matching_node_impl = mp::mp_copy_if_q<Nodes, node_finder<C>>;
        template <char C, class Nodes>
        using matching_node
            = mp::mp_eval_if<mp::mp_empty<matching_node_impl<C, Nodes>>, no_matching_node,
                             mp::mp_front, matching_node_impl<C, Nodes>>;

        // inserts the new node into the list of node, replacing any previous ones if necessary
        template <class NewNode, class Nodes>
        using insert_node
            = mp::mp_push_back<mp::mp_remove_if_q<Nodes, node_finder<NewNode::character>>, NewNode>;

        template <class Rule>
        struct insert_rule_q
        {
            template <class Node>
            using fn = typename Node::template insert_rule<Rule>;
        };

        // inserts the rule into all children
        template <class Rule, class Children>
        using insert_rule_into_children = mp::mp_transform_q<insert_rule_q<Rule>, Children>;

        //=== trie ===//
        template <class TokenSpec>
        class trie
        {
            template <class... Children>
            using children_matcher = detail::children_matcher<TokenSpec, Children...>;
            template <class... Rules>
            using rule_matcher = detail::rule_matcher<TokenSpec, Rules...>;

            //=== nodes ===//
            // a non-terminal node matching the given character
            template <char C, class ChildNodes = mp::mp_list<>>
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

                static constexpr match_result<TokenSpec> match(std::size_t length_so_far,
                                                               const char* str,
                                                               const char* end) noexcept
                {
                    return mp::mp_rename<ChildNodes, children_matcher>::try_match(length_so_far + 1,
                                                                                  str + 1, end);
                }
            };

            // a terminal node terminating token Id with the given character
            template <char C, token_kind_detail::id_type<TokenSpec> Id,
                      class ChildNodes = mp::mp_list<>, class Rules = mp::mp_list<>>
            struct terminal_node
            {
                static constexpr auto is_terminal = true;
                using children                    = ChildNodes;
                static constexpr auto character   = C;

                template <class Child>
                using insert = terminal_node<C, Id, insert_node<Child, ChildNodes>, Rules>;

                template <class Rule>
                using insert_rule = std::conditional_t<
                    Rule::is_conflicting_literal(token_kind<TokenSpec>::from_id(Id)),
                    // if it is conflicting: insert into *this and children
                    terminal_node<C, Id, insert_rule_into_children<Rule, ChildNodes>,
                                  mp::mp_push_back<Rules, Rule>>,
                    // otherwise just into children
                    terminal_node<C, Id, insert_rule_into_children<Rule, ChildNodes>, Rules>>;

                static constexpr match_result<TokenSpec> match(std::size_t length_so_far,
                                                               const char* str,
                                                               const char* end) noexcept
                {
                    ++length_so_far;
                    ++str;

                    // check for a longer match
                    auto child_result
                        = mp::mp_rename<ChildNodes, children_matcher>::try_match(length_so_far, str,
                                                                                 end);
                    if (child_result.is_success())
                        // found a longer match
                        return child_result;

                    // check the conflicting rules
                    // if a longer token match happened, longer token was also conflicting
                    auto rule_result
                        = mp::mp_rename<Rules, rule_matcher>::try_match(length_so_far, str, end);
                    if (rule_result.is_matched())
                        // rule matched something
                        return rule_result;

                    // only then match the token
                    return match_result<TokenSpec>::success(token_kind<TokenSpec>::from_id(Id),
                                                            length_so_far);
                }
            };

            // the root node, with additional rules
            template <class ChildNodes = mp::mp_list<>, class Rules = mp::mp_list<>>
            struct root_node
            {
                static constexpr auto is_terminal = false;
                using children                    = ChildNodes;

                template <class Child>
                using insert = root_node<insert_node<Child, ChildNodes>, Rules>;

                template <class Rule>
                using insert_rule
                    // insert all roots into the root node as well
                    = root_node<insert_rule_into_children<Rule, ChildNodes>,
                                mp::mp_push_back<Rules, Rule>>;

                static constexpr match_result<TokenSpec> try_match(const char* str,
                                                                   const char* end) noexcept
                {
                    // match all literals
                    auto child_result
                        = mp::mp_rename<ChildNodes, children_matcher>::try_match(0, str, end);
                    if (child_result.is_matched())
                        return child_result;

                    // now match all rules
                    auto rule_result = mp::mp_rename<Rules, rule_matcher>::try_match(0, str, end);
                    if (rule_result.is_matched())
                        return rule_result;

                    // nothing matched, error
                    return match_result<TokenSpec>::error(1);
                }
            };

            //=== trie construction ===//
            template <class CurNode, token_kind_detail::id_type<TokenSpec> Id>
            struct insert_literal_impl
            {
                template <char... Chars>
                struct fn;
                template <char C>
                struct fn<C>
                {
                    // the target is the C-child of the current node
                    using target_node = matching_node<C, typename CurNode::children>;
                    // target must not be a terminal itself
                    static_assert(!target_node::is_terminal, "duplicate string insert into trie");

                    // need to turn the target into a terminal node with the same character and
                    // children
                    using new_node = terminal_node<C, Id, typename target_node::children>;

                    using type = typename CurNode::template insert<new_node>;
                };
                template <char Head, char Second, char... Rest>
                struct fn<Head, Second, Rest...>
                {
                    // the target is the Head-child of the current node
                    using target_node = matching_node<Head, typename CurNode::children>;

                    // if the target doesn't exist we need to create a new non-terminal node
                    using actual_target
                        = std::conditional_t<std::is_same<target_node, no_matching_node>::value,
                                             non_terminal_node<Head>, target_node>;

                    // insert the children into the actual target
                    using inserted =
                        typename insert_literal_impl<actual_target, Id>::template fn<Second,
                                                                                     Rest...>::type;

                    // insert the target with children into the current node
                    using type = typename CurNode::template insert<inserted>;
                };
            };

        public:
            // an empty trie
            using empty = root_node<>;

            // inserts a literal token where `String = StringTemplate<Chars...>`
            template <class Root, token_kind_detail::id_type<TokenSpec> Id, typename String>
            using insert_literal =
                typename apply_q_char<insert_literal_impl<Root, Id>, String>::type;

            // inserts a rule
            template <class Root, class Rule>
            using insert_rule = typename Root::template insert_rule<Rule>;
        };
    } // namespace detail
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_DETAIL_TRIE_HPP_INCLUDED
