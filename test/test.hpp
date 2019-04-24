// Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_LEX_TEST_HPP_INCLUDED
#define FOONATHAN_LEX_TEST_HPP_INCLUDED

#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ < 8
#    define FOONATHAN_LEX_TEST_CONSTEXPR
#elif defined(_MSC_VER)
#define FOONATHAN_LEX_TEST_CONSTEXPR
#else
#    define FOONATHAN_LEX_TEST_CONSTEXPR constexpr
#endif

#endif // FOONATHAN_LEX_TEST_HPP_INCLUDED
