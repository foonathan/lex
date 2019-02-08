// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_DETAIL_ASSERT_HPP_INCLUDED
#define FOONATHAN_LEX_DETAIL_ASSERT_HPP_INCLUDED

#include <debug_assert.hpp>

#ifndef FOONATHAN_LEX_ENABLE_ASSERTIONS
#    define FOONATHAN_LEX_ENABLE_ASSERTIONS 0
#endif

#ifndef FOONATHAN_LEX_ENABLE_PRECONDITIONS
#    ifdef NDEBUG
#        define FOONATHAN_LEX_ENABLE_PRECONDITIONS 0
#    else
#        define FOONATHAN_LEX_ENABLE_PRECONDITIONS 1
#    endif
#endif

namespace foonathan
{
namespace lex
{
    namespace detail
    {
        struct assert_handler
        : debug_assert::default_handler,
          debug_assert::set_level<static_cast<unsigned>(FOONATHAN_LEX_ENABLE_ASSERTIONS)>
        {};

#define FOONATHAN_LEX_ASSERT(Expr)                                                                 \
    if (detail::assert_handler::level > 0 && !(Expr))                                              \
    DEBUG_UNREACHABLE(detail::assert_handler{}, "internal assertion error: " #Expr)

        struct precondition_handler
        : debug_assert::default_handler,
          debug_assert::set_level<static_cast<unsigned>(FOONATHAN_LEX_ENABLE_PRECONDITIONS)>
        {};

#define FOONATHAN_LEX_PRECONDITION(Expr, Str)                                                      \
    if (detail::precondition_handler::level > 0 && !(Expr))                                        \
    DEBUG_UNREACHABLE(detail::precondition_handler{}, #Expr ": " Str)
    } // namespace detail
} // namespace lex
} // namespace foonathan

#endif // FOONATHAN_LEX_DETAIL_ASSERT_HPP_INCLUDED
