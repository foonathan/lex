// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_WHITESPACE_TOKEN_HPP_INCLUDED
#define FOONATHAN_LEX_WHITESPACE_TOKEN_HPP_INCLUDED

#include <type_traits>

namespace foonathan
{
namespace lex
{
    /// A token that is whitespace and will be ignored.
    ///
    /// Calling `bump()` on the [lex::tokenizer]() will skip past all whitespace tokens.
    /// \notes A token must inherit from this class *in addition* to other token class.
    struct whitespace_token
    {};

    /// Whether or not the token is a whitespace token.
    template <class Token>
    struct is_whitespace : std::is_base_of<whitespace_token, Token>
    {};
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_WHITESPACE_TOKEN_HPP_INCLUDED
