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
        detail/trie.cpp)

add_executable(foonathan_lex_test
               test.cpp
               ${tests})
target_include_directories(foonathan_lex_test SYSTEM PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(foonathan_lex_test PUBLIC foonathan_lex)
set_target_properties(foonathan_lex_test PROPERTIES CXX_STANDARD 14)

add_test(NAME test COMMAND foonathan_lex_test)
