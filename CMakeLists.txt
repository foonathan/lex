# Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

cmake_minimum_required(VERSION 3.8)
project(foonathan_lex VERSION 0.0.0)

include(external/external.cmake)

# source files
set(detail_header_files
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/lex/detail/assert.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/lex/detail/select_integer.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/lex/detail/string.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/lex/detail/trie.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/lex/detail/type_list.hpp
    )
set(header_files
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/lex/ascii.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/lex/identifier_token.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/lex/literal_token.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/lex/match_result.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/lex/rule_token.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/lex/spelling.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/lex/token.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/lex/token_kind.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/lex/token_spec.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/lex/tokenizer.hpp)

# main target
add_library(foonathan_lex INTERFACE)
add_library(foonathan::foonathan_lex ALIAS foonathan_lex)
target_sources(foonathan_lex INTERFACE $<BUILD_INTERFACE:${detail_header_files} ${header_files}>)
target_compile_features(foonathan_lex INTERFACE cxx_std_14)
target_include_directories(foonathan_lex SYSTEM INTERFACE
                                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
                                $<INSTALL_INTERFACE:include/>)
target_link_libraries(foonathan_lex INTERFACE debug_assert)

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
