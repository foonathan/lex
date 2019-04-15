# Copyright (C) 2018-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

add_executable(foonathan_lex_ctokenizer ctokenizer.cpp)
target_link_libraries(foonathan_lex_ctokenizer PUBLIC foonathan_lex)

add_executable(foonathan_lex_calculator calculator.cpp)
target_link_libraries(foonathan_lex_calculator PUBLIC foonathan_lex)
