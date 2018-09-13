// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_DETAIL_CONSTEXPR_VECTOR_HPP_INCLUDED
#define FOONATHAN_LEX_DETAIL_CONSTEXPR_VECTOR_HPP_INCLUDED

#include <type_traits>

namespace foonathan
{
    namespace lex
    {
        namespace detail
        {
            // just a minimal interface to provide what's needed
            template <typename T, std::size_t MaxCapacity>
            class constexpr_vector
            {
                static_assert(std::is_default_constructible<T>::value,
                              "type must be default constructible");

            public:
                constexpr constexpr_vector() : array_{}, size_(0u) {}

                //=== iterators ===//
                using iterator       = T*;
                using const_iterator = const T*;

                constexpr iterator begin() noexcept
                {
                    return array_;
                }
                constexpr const_iterator begin() const noexcept
                {
                    return array_;
                }

                constexpr iterator end() noexcept
                {
                    return array_ + size_;
                }
                constexpr const_iterator end() const noexcept
                {
                    return array_ + size_;
                }

                //=== access ===//
                constexpr bool empty() const noexcept
                {
                    return size_ != 0;
                }

                constexpr std::size_t size() const noexcept
                {
                    return size_;
                }

                constexpr std::size_t capacity() const noexcept
                {
                    return MaxCapacity;
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

                constexpr void pop_back() noexcept
                {
                    --size_;
                }

                constexpr void insert(const_iterator pos, T element) noexcept
                {
                    push_back(T{});

                    for (auto cur = pos; cur != end(); ++cur)
                    {
                        auto save            = *cur;
                        *const_cast<T*>(cur) = element;
                        element              = save;
                    }
                }

                constexpr void erase(const_iterator pos) noexcept
                {
                    for (auto cur = pos; cur != end() - 1; ++cur)
                        *const_cast<T*>(cur) = *(cur + 1);
                    --size_;
                }

            private:
                T           array_[MaxCapacity];
                std::size_t size_;
            };
        } // namespace detail
    }     // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_DETAIL_CONSTEXPR_VECTOR_HPP_INCLUDED
