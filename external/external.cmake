# Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

option(FOONATHAN_LEX_FORCE_FIND_PACKAGE "force find_package() instead of using git submodule" OFF)
set(dependency_via_submodule OFF)

if(FOONATHAN_LEX_FORCE_FIND_PACKAGE)
    find_package(debug_assert REQUIRED)
else()
    find_package(debug_assert QUIET)
    if(NOT debug_assert_FOUND)
        set(dependency_via_submodule ON)
        if(TARGET debug_assert)
            message(STATUS "Using inherited debug_assert target")
        else()
            message(STATUS "Installing debug_assert via submodule")
            execute_process(COMMAND git submodule update --init -- external/debug_assert
                            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
            add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/debug_assert EXCLUDE_FROM_ALL)
        endif()
    endif()
endif()
