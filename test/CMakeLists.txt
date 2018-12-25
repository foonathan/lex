# Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

# get Catch
if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/catch.hpp)
    file(DOWNLOAD
         https://raw.githubusercontent.com/catchorg/Catch2/master/single_include/catch2/catch.hpp
         ${CMAKE_CURRENT_BINARY_DIR}/catch.hpp
         STATUS status
         LOG log)

    list(GET status 0 status_code)
    list(GET status 1 status_string)

    if(NOT status_code EQUAL 0)
        message(FATAL_ERROR "error downloading catch: ${status_string}"
                "${log}")
    endif()
endif()

# define dummy target with all the options
# can't link to foonathan_lex, as it adds a SYSTEM interface
add_library(foonathan_lex_test_base INTERFACE)
target_sources(foonathan_lex_test_base INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/test.cpp)
target_include_directories(foonathan_lex_test_base INTERFACE
                                ${CMAKE_CURRENT_SOURCE_DIR}
                                ${CMAKE_CURRENT_SOURCE_DIR}/../include
                            SYSTEM INTERFACE
                                ${CMAKE_CURRENT_BINARY_DIR})
target_compile_features(foonathan_lex_test_base INTERFACE cxx_std_14)
target_link_libraries(foonathan_lex_test_base INTERFACE debug_assert)
target_compile_definitions(foonathan_lex_test_base INTERFACE
                               FOONATHAN_LEX_TEST
                               FOONATHAN_LEX_ENABLE_ASSERTIONS=1
                               FOONATHAN_LEX_ENABLE_PRECONDITIONS=1)
target_compile_options(foonathan_lex_test_base INTERFACE
                           # clang/GCC warnings
                           $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:
                           -pedantic-errors -Werror -Wall -Wextra -Wconversion -Wsign-conversion>
                           # disable noexcept type warning on GCC
                           $<$<CXX_COMPILER_ID:GNU>: -Wno-noexcept-type>
                           # MSVC warnings
                           $<$<CXX_COMPILER_ID:MSVC>:
                           /WX /W4>)

# the unit tests
set(tests
    detail/string.cpp
    detail/trie.cpp
    ascii.cpp
    identifier_token.cpp
    literal_token.cpp
    rule_token.cpp
    tokenizer.cpp
    whitespace_token.cpp)

add_executable(foonathan_lex_test tokenize.hpp ${tests})
target_link_libraries(foonathan_lex_test PUBLIC foonathan_lex_test_base)
add_test(NAME test COMMAND foonathan_lex_test)

# test case to ensure the ctokenizer works
add_executable(foonathan_lex_ctokenizer_test ../example/ctokenizer.cpp)
target_link_libraries(foonathan_lex_ctokenizer_test PUBLIC foonathan_lex_test_base)
add_test(NAME ctokenizer COMMAND foonathan_lex_ctokenizer_test)
