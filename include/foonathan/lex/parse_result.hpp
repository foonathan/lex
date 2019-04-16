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

    namespace detail
    {
        struct unmatched_tag
        {};

        template <typename T, typename = void>
        struct parse_result_storage
        {
            union
            {
                T    matched_;
                char unmatched_;
            };
            bool is_matched_;

            explicit parse_result_storage(unmatched_tag) noexcept : unmatched_{}, is_matched_(false)
            {}

            explicit parse_result_storage(T&& result) noexcept(
                std::is_nothrow_move_constructible<T>::value)
            : matched_(static_cast<T&&>(result)), is_matched_(true)
            {}

            ~parse_result_storage() noexcept
            {
                if (is_matched_)
                    matched_.~T();
            }
        };
        template <typename T>
        struct parse_result_storage<T, std::enable_if_t<std::is_trivially_destructible<T>::value>>
        {
            union
            {
                T    matched_;
                char unmatched_;
            };
            bool is_matched_;

            constexpr explicit parse_result_storage(unmatched_tag) noexcept
            : unmatched_{}, is_matched_(false)
            {}

            constexpr explicit parse_result_storage(T&& result) noexcept(
                std::is_nothrow_move_constructible<T>::value)
            : matched_(static_cast<T&&>(result)), is_matched_(true)
            {}

            ~parse_result_storage() noexcept = default;
        };

        template <typename T, typename = void>
        struct parse_result_impl : parse_result_storage<T>
        {
            using parse_result_storage<T>::parse_result_storage;

            parse_result_impl(parse_result_impl&& other) noexcept(
                std::is_nothrow_move_constructible<T>::value)
            : parse_result_storage<T>(unmatched_tag{})
            {
                this->is_matched_ = other.is_matched_;
                if (other.is_matched_)
                    ::new (static_cast<void*>(&this->matched_)) T(static_cast<T&&>(other.matched_));
            }

            parse_result_impl& operator=(parse_result_impl&& other) noexcept(
                std::is_nothrow_move_assignable<T>::value)
            {
                if (this->is_matched_)
                {
                    if (other.is_matched_)
                        this->matched_ = static_cast<T&&>(other.matched_);
                    else
                    {
                        this->matched_.~T();
                        this->is_matched_ = false;
                    }
                }
                else if (other.is_matched_)
                {
                    ::new (static_cast<void*>(&this->matched_)) T(static_cast<T&&>(other.matched_));
                    this->is_matched_ = true;
                }

                return *this;
            }
        };
        template <typename T>
        struct parse_result_impl<T,
                                 std::enable_if_t<std::is_trivially_move_constructible<T>::value
                                                  && std::is_trivially_move_assignable<T>::value>>
        : parse_result_storage<T>
        {
            using parse_result_storage<T>::parse_result_storage;

            constexpr parse_result_impl(parse_result_impl&& other) noexcept = default;
            constexpr parse_result_impl& operator=(parse_result_impl&& other) noexcept = default;
        };
    } // namespace detail

    /// The result of parsing a production.
    ///
    /// It either matched resulting in `T` or was not matched.
    template <typename T>
    class parse_result : detail::parse_result_impl<T>
    {
    public:
        using value_type = T;

        static constexpr parse_result<T> unmatched()
        {
            return parse_result<T>(detail::unmatched_tag{});
        }

        static constexpr parse_result<T> success(T&& result)
        {
            return parse_result<T>(static_cast<T&&>(result));
        }

        constexpr parse_result() noexcept : parse_result(detail::unmatched_tag{}) {}

        /// \returns Whether or not nothing was matched at all.
        constexpr bool is_unmatched() const noexcept
        {
            return !this->is_matched_;
        }

        /// \returns Whether or not the result is a success.
        constexpr bool is_success() const noexcept
        {
            return this->is_matched_;
        }

        constexpr T& value() & noexcept
        {
            FOONATHAN_LEX_PRECONDITION(is_success(), "must be a success");
            return this->matched_;
        }
        constexpr const T& value() const& noexcept
        {
            FOONATHAN_LEX_PRECONDITION(is_success(), "must be a success");
            return this->matched_;
        }
        constexpr T&& value() && noexcept
        {
            FOONATHAN_LEX_PRECONDITION(is_success(), "must be a success");
            return static_cast<T&&>(this->matched_);
        }
        constexpr const T&& value() const&& noexcept
        {
            FOONATHAN_LEX_PRECONDITION(is_success(), "must be a success");
            return static_cast<T&&>(this->matched_);
        }

        template <class Production>
        constexpr T&& forward(Production = {}) noexcept
        {
            FOONATHAN_LEX_PRECONDITION(is_success(), "must be a success");
            return static_cast<T&&>(this->matched_);
        }

    private:
        using detail::parse_result_impl<T>::parse_result_impl;
    };

    template <>
    class parse_result<void>
    {
    public:
        using value_type = void;

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
        constexpr Production forward(Production = {}) noexcept
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
