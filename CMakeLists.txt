# Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

cmake_minimum_required(VERSION 3.1)
project(foonathan_lex)

set(detail_header_files
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/lex/detail/constexpr_vector.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/include/foonathan/lex/detail/trie.hpp
    )
set(header_files)
add_library(foonathan_lex INTERFACE)
target_sources(foonathan_lex INTERFACE ${detail_header_files} ${header_files})
target_include_directories(foonathan_lex INTERFACE include/)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    enable_testing()
    add_subdirectory(test)
endif()

# TODO: temporary, for testing
add_executable(foonathan_lex_main main.cpp)
target_link_libraries(foonathan_lex_main PUBLIC foonathan_lex)
