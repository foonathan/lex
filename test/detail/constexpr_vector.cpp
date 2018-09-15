// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <foonathan/lex/detail/constexpr_vector.hpp>

#include <catch.hpp>

using namespace foonathan::lex;

namespace
{
using test_vector = detail::constexpr_vector<int, 16>;

void verify(const test_vector& vec, std::initializer_list<int> values)
{
    REQUIRE(vec.size() == values.size());
    for (auto i = 0u; i != vec.size(); ++i)
        REQUIRE(vec[i] == values.begin()[i]);
}

constexpr test_vector test_push_back(test_vector input)
{
    input.push_back(1);
    input.push_back(2);
    input.push_back(3);
    input.push_back(4);

    return input;
}

constexpr test_vector test_pop_back(test_vector input)
{
    input.pop_back();
    input.pop_back();

    return input;
}

constexpr test_vector test_insert(test_vector input)
{
    input.insert(input.begin(), 11);
    input.insert(input.begin() + 2, 22);

    return input;
}

constexpr test_vector test_erase(test_vector input)
{
    input.erase(input.begin());
    input.erase(input.begin() + 1);

    return input;
}
} // namespace

TEST_CASE("detail::constexpr_vector")
{
    constexpr auto vec0 = test_vector();
    verify(vec0, {});

    constexpr auto vec1 = test_push_back(vec0);
    verify(vec1, {1, 2, 3, 4});

    constexpr auto vec2 = test_pop_back(vec1);
    verify(vec2, {1, 2});

    constexpr auto vec3 = test_insert(vec2);
    verify(vec3, {11, 1, 22, 2});

    constexpr auto vec4 = test_erase(vec3);
    verify(vec4, {1, 2});
}
