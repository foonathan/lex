# Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

message(STATUS "Installing google/benchmark via submodule")
execute_process(COMMAND git submodule update --init -- ../external/benchmark
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "")
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "")
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../external/benchmark ${CMAKE_CURRENT_BINARY_DIR}/../external/benchmark
                 EXCLUDE_FROM_ALL)

add_executable(foonathan_lex_benchmark
               benchmark.cpp
               bm_baseline.hpp
               bm_manual.hpp
               bm_manual_opt.hpp
               bm_tokenizer.hpp
               bm_tokenizer_manual.hpp
               bm_trie.hpp)
target_link_libraries(foonathan_lex_benchmark PUBLIC foonathan_lex benchmark)
target_compile_definitions(foonathan_lex_benchmark PUBLIC
                           FOONATHAN_LEX_ENABLE_ASSERTIONS=0
                           FOONATHAN_LEX_ENABLE_PRECONDITIONS=0)
