// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_TOKEN_SPEC_HPP_INCLUDED
#define FOONATHAN_LEX_TOKEN_SPEC_HPP_INCLUDED

#include <foonathan/lex/detail/type_list.hpp>

namespace foonathan
{
namespace lex
{
    template <class... Tokens>
    using token_spec = detail::type_list<Tokens...>;

    /// \exclude
    namespace production_rule
    {
        struct production_adl
        {};
    } // namespace production_rule

    namespace detail
    {
        struct base_token : production_rule::production_adl
        {};
    } // namespace detail

    template <typename T>
    struct is_token : std::is_base_of<detail::base_token, T>
    {};

    struct error_token : detail::base_token
    {};

    struct eof_token : detail::base_token
    {};
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_TOKEN_SPEC_HPP_INCLUDED
