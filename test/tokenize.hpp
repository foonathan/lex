// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_TOKENIZE_HPP_INCLUDED
#define FOONATHAN_LEX_TOKENIZE_HPP_INCLUDED

#include <foonathan/lex/tokenizer.hpp>

namespace
{
namespace lex = foonathan::lex;

// just a minimal interface to provide what's needed
template <typename T, std::size_t MaxCapacity>
class constexpr_vector
{
    static_assert(std::is_default_constructible<T>::value, "type must be default constructible");

public:
    constexpr constexpr_vector() : array_{}, size_(0u) {}

    //=== access ===//
    constexpr bool empty() const noexcept
    {
        return size_ != 0;
    }

    constexpr std::size_t size() const noexcept
    {
        return size_;
    }

    constexpr T& operator[](std::size_t i) noexcept
    {
        return array_[i];
    }
    constexpr const T& operator[](std::size_t i) const noexcept
    {
        return array_[i];
    }

    //=== modifiers ===//
    constexpr void push_back(T element) noexcept
    {
        array_[size_] = element;
        ++size_;
    }

private:
    T           array_[MaxCapacity];
    std::size_t size_;
};

template <class Spec>
using vector = constexpr_vector<lex::token<Spec>, 32>;

#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ < 8
#    define FOONATHAN_LEX_TEST_CONSTEXPR
#else
#    define FOONATHAN_LEX_TEST_CONSTEXPR constexpr
#endif

template <class Spec>
FOONATHAN_LEX_TEST_CONSTEXPR vector<Spec> tokenize(lex::tokenizer<Spec> tokenizer)
{
    vector<Spec> result;

    while (!tokenizer.is_done())
        result.push_back(tokenizer.get());

    return result;
}
} // namespace

#endif // FOONATHAN_LEX_TOKENIZE_HPP_INCLUDED
