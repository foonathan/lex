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
    /// The specification of the tokens.
    ///
    /// Every type must be derived from one of the token classes and describes how one token should
    /// be parsed. The [lex::tokenizer]() will implement that parsing.
    template <class... Tokens>
    using token_spec = detail::type_list<Tokens...>;

    namespace detail
    {
        struct base_token
        {};
    } // namespace detail

    /// Whether or not the given type is a token.
    template <typename T>
    struct is_token : std::is_base_of<detail::base_token, T>
    {};

    /// Tag type to mark an error token.
    struct error_token : detail::base_token
    {};

    /// Tag type to mark the EOF token.
    /// It is generated at the very end.
    struct eof_token : detail::base_token
    {};
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_TOKEN_SPEC_HPP_INCLUDED
