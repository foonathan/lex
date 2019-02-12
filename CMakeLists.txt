# Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

cmake_minimum_required(VERSION 3.8)
if(POLICY CMP0076)
    cmake_policy(SET CMP0076 NEW)
    set(source_prefix ".")
else()
    set(source_prefix "${CMAKE_CURRENT_SOURCE_DIR}")
endif()

project(foonathan_lex VERSION 0.0.0)

include(external/external.cmake)

# source files

# main target
add_library(foonathan_lex INTERFACE)
add_library(foonathan::foonathan_lex ALIAS foonathan_lex)
target_compile_features(foonathan_lex INTERFACE cxx_std_14)
target_include_directories(foonathan_lex SYSTEM INTERFACE
                           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
                           $<INSTALL_INTERFACE:include/>)
target_link_libraries(foonathan_lex INTERFACE debug_assert)

target_sources(foonathan_lex INTERFACE 
                ${source_prefix}/include/foonathan/lex/detail/assert.hpp
                ${source_prefix}/include/foonathan/lex/detail/assert.hpp
                ${source_prefix}/include/foonathan/lex/detail/select_integer.hpp
                ${source_prefix}/include/foonathan/lex/detail/string.hpp
                ${source_prefix}/include/foonathan/lex/detail/trie.hpp
                ${source_prefix}/include/foonathan/lex/detail/type_list.hpp
               )
target_sources(foonathan_lex INTERFACE
                ${source_prefix}/include/foonathan/lex/ascii.hpp
                ${source_prefix}/include/foonathan/lex/identifier_token.hpp
                ${source_prefix}/include/foonathan/lex/literal_token.hpp
                ${source_prefix}/include/foonathan/lex/match_result.hpp
                ${source_prefix}/include/foonathan/lex/rule_token.hpp
                ${source_prefix}/include/foonathan/lex/spelling.hpp
                ${source_prefix}/include/foonathan/lex/token.hpp
                ${source_prefix}/include/foonathan/lex/token_kind.hpp
                ${source_prefix}/include/foonathan/lex/token_spec.hpp
                ${source_prefix}/include/foonathan/lex/tokenizer.hpp
                ${source_prefix}/include/foonathan/lex/whitespace_token.hpp
               )

# installation
if(NOT dependency_via_submodule)
    include(CMakePackageConfigHelpers)
    write_basic_package_version_file(foonathan_lex-config-version.cmake COMPATIBILITY ExactVersion)
    install(TARGETS foonathan_lex EXPORT foonathan_lex_targets
                INCLUDES DESTINATION include)
    install(EXPORT foonathan_lex_targets
                DESTINATION lib/cmake/foonathan_lex
                FILE foonathan_lex-targets.cmake
                NAMESPACE foonathan::)
    install(DIRECTORY include/
                DESTINATION include)
    install(FILES foonathan_lex-config.cmake ${CMAKE_CURRENT_BINARY_DIR}/foonathan_lex-config-version.cmake
                DESTINATION lib/cmake/foonathan_lex)
else()
    message(STATUS "Dependency installed via submodule, installation unavailable")
endif()

# subdirectories
option(FOONATHAN_LEX_BUILD_TEST "build tests of foonathan/lex" OFF)
if(${FOONATHAN_LEX_BUILD_TEST} OR (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR))
    enable_testing()
    add_subdirectory(test)
endif()

option(FOONATHAN_LEX_BUILD_BENCHMARK "build benchmarks of foonathan/lex" OFF)
if(${FOONATHAN_LEX_BUILD_TEST} OR (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR))
    add_subdirectory(benchmark)
endif()

option(FOONATHAN_LEX_BUILD_EXAMPLE "build examples of foonathan/lex" OFF)
if(${FOONATHAN_LEX_BUILD_EXAMPLE} OR (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR))
    add_subdirectory(example)
endif()
