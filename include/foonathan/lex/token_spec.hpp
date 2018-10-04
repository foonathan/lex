// Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
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
    template <class... Tokens>
    using token_spec = detail::type_list<Tokens...>;

    /// Tag type to mark an error token.
    struct error_token
    {};

    /// Tag type to mark the EOF token.
    /// It is generated at the very end.
    struct eof_token
    {};
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_TOKEN_SPEC_HPP_INCLUDED
