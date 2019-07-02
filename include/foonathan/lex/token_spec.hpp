// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_TOKEN_SPEC_HPP_INCLUDED
#define FOONATHAN_LEX_TOKEN_SPEC_HPP_INCLUDED

#include <boost/mp11/list.hpp>

namespace foonathan
{
namespace lex
{
    namespace production_rule
    {
        struct production_adl
        {};
    } // namespace production_rule
    namespace token_regex
    {
        struct regex_adl
        {};
    } // namespace token_regex

    namespace detail
    {
        struct base_token : production_rule::production_adl, token_regex::regex_adl
        {};
    } // namespace detail

    template <typename T>
    struct is_token : std::is_base_of<detail::base_token, T>
    {};

    struct error_token : detail::base_token
    {
        static constexpr const char* name = "<error>";
    };

    struct eof_token : detail::base_token
    {
        static constexpr const char* name = "<eof>";
    };

    template <class... Tokens>
    struct token_spec
    {
        using list = boost::mp11::mp_list<error_token, Tokens..., eof_token>;
    };
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_TOKEN_SPEC_HPP_INCLUDED
