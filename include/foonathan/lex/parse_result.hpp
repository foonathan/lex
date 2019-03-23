// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_PARSE_RESULT_HPP_INCLUDED
#define FOONATHAN_LEX_PARSE_RESULT_HPP_INCLUDED

#include <type_traits>

#include <foonathan/lex/detail/assert.hpp>

namespace foonathan
{
namespace lex
{
    template <class Production>
    struct callback_result_of
    {};

    /// The result of parsing a production.
    ///
    /// It either matched resulting in `T` or was not matched.
    template <typename T>
    class parse_result
    {
        static_assert(std::is_trivially_copyable<T>::value, "TODO"); // TODO

    public:
        static constexpr parse_result<T> unmatched()
        {
            return parse_result<T>(unmatched_tag{});
        }

        static constexpr parse_result<T> success(T&& result)
        {
            return parse_result<T>(static_cast<T&&>(result));
        }

        constexpr parse_result() noexcept : parse_result(unmatched_tag{}) {}

        /// \returns Whether or not nothing was matched at all.
        constexpr bool is_unmatched() const noexcept
        {
            return !is_matched_;
        }

        /// \returns Whether or not the result is a success.
        constexpr bool is_success() const noexcept
        {
            return is_matched_;
        }

        constexpr const T& value() const noexcept
        {
            FOONATHAN_LEX_PRECONDITION(is_success(), "must be a success");
            return matched_;
        }

        template <class Production>
        constexpr T value_or_tag(Production = {}) const noexcept
        {
            FOONATHAN_LEX_PRECONDITION(is_success(), "must be a success");
            return matched_;
        }

    private:
        struct unmatched_tag
        {};
        constexpr explicit parse_result(unmatched_tag) noexcept : unmatched_{}, is_matched_(false)
        {}

        constexpr explicit parse_result(T&& result)
        : matched_(static_cast<T&&>(result)), is_matched_(true)
        {}

        union
        {
            T    matched_;
            char unmatched_;
        };
        bool is_matched_;
    };

    template <>
    class parse_result<void>
    {
    public:
        static constexpr parse_result<void> unmatched()
        {
            return parse_result<void>(false);
        }
        static constexpr parse_result<void> success()
        {
            return parse_result<void>(true);
        }

        constexpr parse_result() noexcept : parse_result(false) {}

        /// \returns Whether or not nothing was matched at all.
        constexpr bool is_unmatched() const noexcept
        {
            return !is_matched_;
        }

        /// \returns Whether or not the result is a success.
        constexpr bool is_success() const noexcept
        {
            return is_matched_;
        }

        template <class Production>
        constexpr Production value_or_tag(Production = {}) const noexcept
        {
            FOONATHAN_LEX_PRECONDITION(is_success(), "must be a success");
            return {};
        }

    private:
        constexpr explicit parse_result(bool matched) noexcept : is_matched_(matched) {}

        bool is_matched_;
    };

    namespace detail
    {
        template <class Return, class Func, typename... Args>
        constexpr auto apply_parse_result_impl(Func& f, Args&&... args)
            -> std::enable_if_t<!std::is_same<void, Return>::value, parse_result<Return>>
        {
            return parse_result<Return>::success(f(static_cast<Args&&>(args)...));
        }
        template <class Return, class Func, typename... Args>
        constexpr auto apply_parse_result_impl(Func& f, Args&&... args)
            -> std::enable_if_t<std::is_same<void, Return>::value, parse_result<Return>>
        {
            f(static_cast<Args&&>(args)...);
            return parse_result<Return>::success();
        }

        template <typename... Args>
        constexpr bool required_signature = false;
        template <typename... Args>
        struct missing_callback_overload
        {
            static_assert(required_signature<Args...>, "missing callback overload");
        };

        template <class Production>
        constexpr bool for_production = false;
        template <class Func, class Production>
        struct missing_callback_result_of
        {
            static_assert(for_production<Production>, "need a callback_result_of overload");
        };

        template <typename Func, typename... Args>
        auto apply_return_type(int, Func& f, Args&&... args)
            -> decltype(f(static_cast<Args&&>(args)...));
        template <typename Func, typename... Args>
        auto apply_return_type(short, Func&, Args&&...) -> missing_callback_overload<Args&&...>;

        template <typename Func, typename... Args>
        constexpr auto apply_parse_result(Func& f, Args&&... args)
        {
            using type = decltype(apply_return_type(0, f, static_cast<Args&&>(args)...));
            return apply_parse_result_impl<type>(f, static_cast<Args&&>(args)...);
        }
    } // namespace detail
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_PARSE_RESULT_HPP_INCLUDED
