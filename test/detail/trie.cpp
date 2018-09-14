// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/lex/detail/trie.hpp>

#include <cstring>
#include <catch.hpp>

using namespace foonathan::lex;

namespace
{
    using test_trie = detail::trie<const char*, 16>;

    void verify(const test_trie& trie, const char* str, const char* prefix)
    {
        auto result = trie.lookup_prefix(str, std::strlen(str));
        REQUIRE(result);
        REQUIRE(std::strcmp(result.data, prefix) == 0);
        REQUIRE(result.prefix_length == std::strlen(result.data));
    }

    bool test_failure()
    {
        return true;
    }

    constexpr test_trie insert_single(test_trie trie)
    {
        trie.insert("a", "a") || test_failure();
        trie.insert("b", "b") || test_failure();
        trie.insert("c", "c") || test_failure();
        return trie;
    }

    constexpr test_trie insert_multiple(test_trie trie)
    {
        trie.insert("ab", "ab") || test_failure();
        trie.insert("abcd", "abcd") || test_failure();
        trie.insert("bc", "bc") || test_failure();
        return trie;
    }

    constexpr const char* test_lookup(const test_trie& trie)
    {
        return trie.lookup_prefix("a", 1).data;
    }
} // namespace

TEST_CASE("detail::trie")
{
    constexpr auto trie0 = test_trie();
    REQUIRE(!trie0.lookup_prefix("a", 1));

    constexpr auto trie1 = insert_single(trie0);
    verify(trie1, "a", "a");
    verify(trie1, "b", "b");
    verify(trie1, "c", "c");
    verify(trie1, "ab", "a");
    REQUIRE(!trie1.lookup_prefix("d", 1));

    constexpr auto trie2 = insert_multiple(trie1);
    verify(trie2, "a", "a");
    verify(trie2, "ab", "ab");
    verify(trie2, "abcd", "abcd");
    verify(trie2, "abc", "ab");
    verify(trie2, "b", "b");
    verify(trie2, "bc", "bc");
    verify(trie2, "bcd", "bc");
    verify(trie2, "c", "c");
    verify(trie2, "cd", "c");
    REQUIRE(!trie2.lookup_prefix("d", 1));

    constexpr auto result = test_lookup(trie2);
    REQUIRE(std::strcmp(result, "a") == 0);
}
