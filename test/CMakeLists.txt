# Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

# get doctest
if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/doctest_main.h)
    file(DOWNLOAD
         https://raw.githubusercontent.com/onqtam/doctest/master/doctest/doctest.h
         ${CMAKE_CURRENT_BINARY_DIR}/doctest.h
         STATUS status
         LOG log)
    # need separate file for zapcc
    configure_file(${CMAKE_CURRENT_BINARY_DIR}/doctest.h ${CMAKE_CURRENT_BINARY_DIR}/doctest_main.h COPYONLY)

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
                           SYSTEM INTERFACE
                           ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(foonathan_lex_test_base INTERFACE foonathan_lex)
target_compile_definitions(foonathan_lex_test_base INTERFACE
                           FOONATHAN_LEX_TEST
                           FOONATHAN_LEX_ENABLE_ASSERTIONS=1
                           FOONATHAN_LEX_ENABLE_PRECONDITIONS=1)

target_compile_options(foonathan_lex_test_base INTERFACE
                       # clang/GCC warnings
                       $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:
                       -pedantic-errors -Werror -Wall -Wextra -Wconversion -Wsign-conversion>
                       # disable annoying warnings and/or some that create a weird compiler bug
                       $<$<CXX_COMPILER_ID:GNU>: -Wno-parentheses -Wno-noexcept-type -Wno-format -Wno-nonnull -Wno-restrict>
                       # MSVC warnings
                       $<$<CXX_COMPILER_ID:MSVC>:
                       /WX /W4 /wd4127>)

# the unit tests
set(tests
    detail/string.cpp
    detail/trie.cpp
    ascii.cpp
    identifier_token.cpp
    list_production.cpp
    literal_token.cpp
    operator_production.cpp
    production_rule_production.cpp
    production_rule_token.cpp
    rule_token.cpp
    token_regex.cpp
    tokenizer.cpp
    whitespace_token.cpp)

add_executable(foonathan_lex_test tokenize.hpp test.hpp ${tests})
target_link_libraries(foonathan_lex_test PUBLIC foonathan_lex_test_base)
add_test(NAME test COMMAND foonathan_lex_test)

# test case to ensure the ctokenizer works
add_executable(foonathan_lex_ctokenizer_test ../example/ctokenizer.cpp)
target_link_libraries(foonathan_lex_ctokenizer_test PUBLIC foonathan_lex_test_base)
add_test(NAME ctokenizer COMMAND foonathan_lex_ctokenizer_test)
