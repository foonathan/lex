# Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

include(CMakeFindDependencyMacro)
find_dependency(debug_assert)
include("${CMAKE_CURRENT_LIST_DIR}/foonathan_lex-targets.cmake")
