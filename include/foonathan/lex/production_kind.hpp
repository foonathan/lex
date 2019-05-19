// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_PRODUCTION_KIND_HPP_INCLUDED
#define FOONATHAN_LEX_PRODUCTION_KIND_HPP_INCLUDED

#include <boost/mp11/algorithm.hpp>

#include <foonathan/lex/detail/select_integer.hpp>
#include <foonathan/lex/grammar.hpp>

namespace foonathan
{
namespace lex
{
    namespace production_kind_detail
    {
        template <class Grammar>
        using id_type
            = detail::select_integer<boost::mp11::mp_size<typename Grammar::productions>::value>;

        template <class Grammar, class Production>
        constexpr id_type<Grammar> get_id() noexcept
        {
            constexpr auto index
                = boost::mp11::mp_find<typename Grammar::productions, Production>::value;
            static_assert(boost::mp11::mp_contains<typename Grammar::productions,
                                                   Production>::value,
                          "not one of the specified productions");
            return static_cast<id_type<Grammar>>(index);
        }
    } // namespace production_kind_detail

    /// Information about the kind of a production.
    template <class Grammar>
    class production_kind
    {
    public:
        /// \effects Creates it from the integral id.
        static constexpr production_kind from_id(std::size_t id) noexcept
        {
            return production_kind(0, static_cast<production_kind_detail::id_type<Grammar>>(id));
        }

        /// \effects Creates it from an incomplete production type.
        /// But otherwise behaves like the constructor.
        template <class Production>
        static constexpr production_kind of() noexcept
        {
            return production_kind(0, production_kind_detail::get_id<Grammar, Production>());
        }

        /// \effects Creates the specified production kind.
        /// \requires The token must be one of the specified productions.
        template <class Production>
        constexpr production_kind(Production) noexcept
        : id_(production_kind_detail::get_id<Grammar, Production>())
        {}

        /// \returns Whether or not it is the specified production kind.
        template <class Production>
        constexpr bool is(Production = {}) const noexcept
        {
            return id_ == production_kind_detail::get_id<Grammar, Production>();
        }

        /// \returns The underlying integer value of the token.
        constexpr production_kind_detail::id_type<Grammar> get() const noexcept
        {
            return id_;
        }

        friend constexpr bool operator==(production_kind lhs, production_kind rhs) noexcept
        {
            return lhs.id_ == rhs.id_;
        }
        friend constexpr bool operator!=(production_kind lhs, production_kind rhs) noexcept
        {
            return !(lhs == rhs);
        }

    private:
        explicit constexpr production_kind(int,
                                           production_kind_detail::id_type<Grammar> id) noexcept
        : id_(id)
        {}

        production_kind_detail::id_type<Grammar> id_;
    };
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_PRODUCTION_KIND_HPP_INCLUDED
