// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_DETAIL_TRIE_HPP_INCLUDED
#define FOONATHAN_LEX_DETAIL_TRIE_HPP_INCLUDED

#include <foonathan/lex/detail/constexpr_vector.hpp>
#include <foonathan/lex/detail/select_integer.hpp>

namespace foonathan
{
namespace lex
{
    namespace detail
    {
        template <std::size_t MaxNodes>
        using node_index = select_integer<MaxNodes>;

        /// A simple constexpr trie data structure associating strings with `UserData`.
        /// It can contain at most `MaxNodes` nodes.
        template <typename UserData, std::size_t MaxNodes>
        class trie
        {
            static_assert(std::is_default_constructible<UserData>::value,
                          "user data must be default constructible");

            struct node
            {
                static constexpr auto invalid = node_index<MaxNodes>(-1);

                // this maintains a linked list of children
                node_index<MaxNodes> first_child          = invalid;
                node_index<MaxNodes> next_child_of_parent = invalid;

                UserData data{};

                char character = '\0'; // '\0' for root node

                constexpr node() noexcept = default;

                constexpr explicit node(char c) noexcept : character(c) {}

                constexpr bool set_data(UserData data) noexcept
                {
                    if (this->data)
                        return false;

                    this->data = data;
                    return true;
                }

                constexpr const UserData* get_data() const noexcept
                {
                    return data ? &data : nullptr;
                }
            };

        public:
            using user_data = UserData;

            constexpr trie() noexcept
            {
                nodes_.push_back(node{});
            }

            /// Inserts the given string and user data.
            /// Returns whether or not this is a duplicate.
            constexpr bool insert(const char* str, UserData data) noexcept
            {
                auto cur_node = root_node();
                while (auto c = *str++)
                {
                    auto child = find_child(cur_node, c);
                    if (!child)
                        // need to add a new child for the current character
                        child = create_child(cur_node, node(c));

                    cur_node = child;
                }

                return const_cast<node*>(cur_node)->set_data(data);
            }

            struct LookupResult
            {
                UserData    data;
                std::size_t prefix_length;

                explicit constexpr operator bool() const noexcept
                {
                    return prefix_length > 0;
                }
            };

            /// Returns the data of the longest matching prefix.
            constexpr LookupResult lookup_prefix(const char* str, const char* end) const noexcept
            {
                auto cur_node      = root_node();
                auto data          = cur_node->get_data();
                auto prefix_length = std::size_t(0);
                for (auto begin = str; str != end; ++str)
                {
                    auto child = find_child(cur_node, *str);
                    if (!child)
                        // we can no longer extend the prefix
                        break;

                    cur_node = child;
                    if (auto new_data = cur_node->get_data())
                    {
                        data          = new_data;
                        prefix_length = static_cast<std::size_t>(str - begin + 1);
                    }
                }

                // return the last valid data, i.e. the longest prefix
                if (data)
                    return {*data, prefix_length};
                else
                    return {{}, 0};
            }
            constexpr LookupResult lookup_prefix(const char* str, std::size_t length) const noexcept
            {
                return lookup_prefix(str, str + length);
            }

        private:
            constexpr const node* root_node() const noexcept
            {
                return &nodes_[0];
            }

            constexpr node* create_child(const node* parent_, node child) noexcept
            {
                auto parent   = const_cast<node*>(parent_);
                auto new_node = nodes_.push_back(child);

                // insert as first child
                auto old_first                 = parent->first_child;
                parent->first_child            = nodes_.size() - 1;
                new_node->next_child_of_parent = old_first;

                return new_node;
            }

            constexpr const node* find_child(const node* cur, char c) const noexcept
            {
                auto cur_child = cur->first_child;
                while (cur_child != node::invalid)
                {
                    auto& n = nodes_[cur_child];
                    if (n.character == c)
                        return &n;
                    else
                        cur_child = n.next_child_of_parent;
                }

                return nullptr;
            }

            // +1 for root node
            constexpr_vector<node, MaxNodes + 1> nodes_;
        };
    } // namespace detail
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_DETAIL_TRIE_HPP_INCLUDED
