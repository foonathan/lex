// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_SPELLING_HPP_INCLUDED
#define FOONATHAN_LEX_SPELLING_HPP_INCLUDED

#include <cstring>

namespace foonathan
{
namespace lex
{
    /// The spelling of a token.
    ///
    /// This is simply a lightweight `std::string_view` replacement.
    class token_spelling
    {
    public:
        explicit constexpr token_spelling(const char* ptr, std::size_t size) noexcept
        : ptr_(ptr), size_(size)
        {}

        constexpr char operator[](std::size_t i) const noexcept
        {
            return ptr_[i];
        }

        constexpr const char* begin() const noexcept
        {
            return ptr_;
        }

        constexpr const char* end() const noexcept
        {
            return ptr_ + size_;
        }

        constexpr const char* data() const noexcept
        {
            return ptr_;
        }

        constexpr std::size_t size() const noexcept
        {
            return size_;
        }

    private:
        const char* ptr_;
        std::size_t size_;
    };

    inline constexpr bool operator==(token_spelling lhs, token_spelling rhs) noexcept
    {
        if (lhs.size() != rhs.size())
            return false;

        for (auto i = 0u; i != lhs.size(); ++i)
            if (lhs[i] != rhs[i])
                return false;
        return true;
    }
    inline constexpr bool operator!=(token_spelling lhs, token_spelling rhs) noexcept
    {
        return !(lhs == rhs);
    }

    inline constexpr bool operator==(token_spelling lhs, const char* rhs) noexcept
    {
        auto iter = lhs.begin();
        while (iter != lhs.end() && *rhs)
        {
            if (*iter != *rhs)
                return false;
            ++iter;
            ++rhs;
        }

        auto iter_done = iter == lhs.end();
        auto rhs_done  = *rhs == '\0';
        return iter_done == rhs_done;
    }
    inline constexpr bool operator==(const char* lhs, token_spelling rhs) noexcept
    {
        return rhs == lhs;
    }
    inline constexpr bool operator!=(token_spelling lhs, const char* rhs) noexcept
    {
        return !(lhs == rhs);
    }
    inline constexpr bool operator!=(const char* lhs, token_spelling rhs) noexcept
    {
        return !(lhs == rhs);
    }
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_SPELLING_HPP_INCLUDED
