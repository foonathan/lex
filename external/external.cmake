# Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
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

add_library(foonathan_lex_mp11 INTERFACE)
if(FOONATHAN_LEX_FORCE_FIND_PACKAGE)
    find_package(Boost 1.69.0 REQUIRED)
    target_link_libraries(foonathan_lex_mp11 INTERFACE Boost::boost)
else()
    find_package(Boost 1.69.0)
    if(Boost_FOUND)
        target_link_libraries(foonathan_lex_mp11 INTERFACE Boost::boost)
    else()
        set(dependency_via_submodule ON)
        if(TARGET Boost::mp11)
            message(STATUS "Using inherited Boost::mp11 target")
        else()
            message(STATUS "Installing Boost::mp11 via submodule")
            execute_process(COMMAND git submodule update --init -- external/mp11
                            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
            target_include_directories(foonathan_lex_mp11 SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/external/mp11/include)
        endif()
    endif()
endif()
