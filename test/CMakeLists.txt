# Copyright (C) 2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

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

set(tests
    detail/constexpr_vector.cpp
    detail/string.cpp
    detail/trie.cpp
    ascii.cpp
    identifier_token.cpp
    literal_token.cpp
    rule_token.cpp
    tokenizer.cpp)

add_executable(foonathan_lex_test
               test.cpp
               tokenize.hpp
               ${tests})
target_include_directories(foonathan_lex_test SYSTEM PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(foonathan_lex_test PUBLIC foonathan_lex)
target_compile_definitions(foonathan_lex_test PUBLIC
                                FOONATHAN_LEX_ENABLE_ASSERTIONS=1
                                FOONATHAN_LEX_ENABLE_PRECONDITIONS=1)
target_compile_options(foonathan_lex_test PUBLIC
                            # clang/GCC warnings
                            $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:
                                -pedantic-errors -Werror -Wall -Wextra -Wconversion>
                            # MSVC warnings
                            $<$<CXX_COMPILER_ID:MSVC>:
                                /WX /W4>)

add_test(NAME test COMMAND foonathan_lex_test)
