// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_GRAMMAR_HPP_INCLUDED
#define FOONATHAN_LEX_GRAMMAR_HPP_INCLUDED

#include <foonathan/lex/detail/type_list.hpp>

namespace foonathan
{
namespace lex
{
    /// A grammar that is parsed.
    template <class TokenSpec, class StartProduction, class... OtherProductions>
    struct grammar : detail::type_list<StartProduction, OtherProductions...>
    {};

    namespace detail
    {
        struct base_production
        {};
    } // namespace detail

    /// Whether or not the given type is a production.
    template <typename T>
    struct is_production : std::is_base_of<detail::base_production, T>
    {};
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_GRAMMAR_HPP_INCLUDED
