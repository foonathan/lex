// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/lex/detail/trie.hpp>

#include <catch.hpp>
#include <cstring>

using namespace foonathan::lex;

namespace
{
using test_trie = detail::trie<const char*>;

template <class Trie>
void verify(Trie, const char* str, const char* prefix)
{
    auto result = Trie::lookup_prefix(str, std::strlen(str));
    REQUIRE(result);
    REQUIRE(std::strcmp(result.data, prefix) == 0);
    REQUIRE(result.length == std::strlen(result.data));
}

constexpr const char a[] = "a";
constexpr const char b[] = "b";
constexpr const char c[] = "c";

template <class Trie>
struct insert_single_impl
{
    using first  = test_trie::insert<Trie, a, 'a'>;
    using second = test_trie::insert<first, b, 'b'>;
    using third  = test_trie::insert<second, c, 'c'>;
    using type   = third;
};

template <class Trie>
using insert_single = typename insert_single_impl<Trie>::type;

constexpr const char ab[]   = "ab";
constexpr const char abcd[] = "abcd";
constexpr const char bc[]   = "bc";

template <class Trie>
struct insert_multiple_impl
{
    using first  = test_trie::insert<Trie, ab, 'a', 'b'>;
    using second = test_trie::insert<first, abcd, 'a', 'b', 'c', 'd'>;
    using third  = test_trie::insert<second, bc, 'b', 'c'>;
    using type   = third;
};

template <class Trie>
using insert_multiple = typename insert_multiple_impl<Trie>::type;

template <class Trie>
constexpr const char* test_lookup(Trie)
{
    return Trie::lookup_prefix("a", 1).data;
}
} // namespace

TEST_CASE("detail::trie")
{
    using trie0 = test_trie::empty;
    REQUIRE(!trie0::lookup_prefix("a", 1));

    using trie1 = insert_single<trie0>;
    verify(trie1{}, "a", "a");
    verify(trie1{}, "b", "b");
    verify(trie1{}, "c", "c");
    verify(trie1{}, "ab", "a");
    REQUIRE(!trie1::lookup_prefix("d", 1));

    using trie2 = insert_multiple<trie1>;
    verify(trie2{}, "a", "a");
    verify(trie2{}, "ab", "ab");
    verify(trie2{}, "abcd", "abcd");
    verify(trie2{}, "abc", "ab");
    verify(trie2{}, "b", "b");
    verify(trie2{}, "bc", "bc");
    verify(trie2{}, "bcd", "bc");
    verify(trie2{}, "c", "c");
    verify(trie2{}, "cd", "c");
    REQUIRE(!trie2::lookup_prefix("d", 1));

    constexpr auto result = test_lookup(trie2{});
    REQUIRE(std::strcmp(result, "a") == 0);
}
