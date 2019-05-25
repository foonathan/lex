// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/lex/detail/trie.hpp>

#include <doctest.h>
#include <cstring>

using namespace foonathan::lex;

namespace
{
// actual types don't matter for the trie, just the id
using tokens = token_spec<struct a, struct b, struct c, struct ab, struct abcd, struct bc>;
struct a
{};
struct b
{};
struct c
{};
struct ab
{};
struct abcd
{};
struct bc
{};

template <typename T>
constexpr token_kind_detail::id_type<tokens> id_of()
{
    return token_kind<tokens>(T{}).get();
}

using test_trie = detail::trie<tokens>;

template <typename T, class Trie>
void verify(Trie, const char* str, const char* prefix)
{
    auto result = Trie::try_match(str, str + std::strlen(str));
    REQUIRE(result.is_success());
    REQUIRE(result.bump == std::strlen(prefix));
    REQUIRE(result.kind.template is<T>());
}

template <class Trie>
void verify_error(Trie, const char* str)
{
    auto result = Trie::try_match(str, str + std::strlen(str));
    REQUIRE(result.is_error());
}

template <char... C>
struct string
{};

template <class Trie>
struct insert_single_impl
{
    using first  = test_trie::insert_literal<Trie, id_of<a>(), string<'a'>>;
    using second = test_trie::insert_literal<first, id_of<b>(), string<'b'>>;
    using third  = test_trie::insert_literal<second, id_of<c>(), string<'c'>>;
    using type   = third;
};

template <class Trie>
using insert_single = typename insert_single_impl<Trie>::type;

template <class Trie>
struct insert_multiple_impl
{
    using first  = test_trie::insert_literal<Trie, id_of<ab>(), string<'a', 'b'>>;
    using second = test_trie::insert_literal<first, id_of<abcd>(), string<'a', 'b', 'c', 'd'>>;
    using third  = test_trie::insert_literal<second, id_of<bc>(), string<'b', 'c'>>;
    using type   = third;
};

template <class Trie>
using insert_multiple = typename insert_multiple_impl<Trie>::type;

template <class Trie>
constexpr auto test_lookup(Trie)
{
    auto str = "a";
    return Trie::try_match(str, str + 1).kind;
}
} // namespace

TEST_CASE("detail::trie")
{
    using trie0 = test_trie::empty;
    verify_error(trie0{}, "a");

    using trie1 = insert_single<trie0>;
    verify<a>(trie1{}, "a", "a");
    verify<b>(trie1{}, "b", "b");
    verify<c>(trie1{}, "c", "c");
    verify<a>(trie1{}, "ab", "a");
    verify_error(trie1{}, "d");

    using trie2 = insert_multiple<trie1>;
    verify<a>(trie2{}, "a", "a");
    verify<ab>(trie2{}, "ab", "ab");
    verify<abcd>(trie2{}, "abcd", "abcd");
    verify<ab>(trie2{}, "abc", "ab");
    verify<b>(trie2{}, "b", "b");
    verify<bc>(trie2{}, "bc", "bc");
    verify<bc>(trie2{}, "bcd", "bc");
    verify<c>(trie2{}, "c", "c");
    verify<c>(trie2{}, "cd", "c");
    verify_error(trie2{}, "d");

    constexpr auto result = test_lookup(trie2{});
    REQUIRE(result.is<a>());
}
