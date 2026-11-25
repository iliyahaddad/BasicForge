cmake_minimum_required(VERSION 3.20)

if(MSVC OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(/W4 /WX- /GR- /EHs-c- /GS- /O2 /Ob2)
endif()

# Shared test helpers (TestSink, lex_all)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Lexer tests
add_executable(lexer_tests
    lexer/lexer_tests.cpp
)
target_link_libraries(lexer_tests PRIVATE bfc_frontend)
target_include_directories(lexer_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
add_test(NAME lexer_tests COMMAND lexer_tests)

# Parser tests
add_executable(parser_tests
    parser/parser_tests.cpp
)
target_link_libraries(parser_tests PRIVATE bfc_frontend)
target_include_directories(parser_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
add_test(NAME parser_tests COMMAND parser_tests)

# Semantic analysis tests
add_executable(semantic_tests
    semantic/semantic_tests.cpp
)
target_link_libraries(semantic_tests PRIVATE bfc_frontend)
target_include_directories(semantic_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
add_test(NAME semantic_tests COMMAND semantic_tests)

# IR builder tests
add_executable(ir_tests
    ir/ir_tests.cpp
)
target_link_libraries(ir_tests PRIVATE bfc_frontend)
target_include_directories(ir_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
add_test(NAME ir_tests COMMAND ir_tests)

# Diagnostic tests
add_executable(diagnostic_tests
    diagnostics/diagnostic_tests.cpp
)
target_link_libraries(diagnostic_tests PRIVATE bfc_frontend)
target_include_directories(diagnostic_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
add_test(NAME diagnostic_tests COMMAND diagnostic_tests)

# Integration tests (end-to-end)
add_executable(integration_tests
    integration/integration_tests.cpp
)
target_include_directories(integration_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
add_test(NAME integration_tests COMMAND integration_tests)
set_tests_properties(integration_tests PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    ENVIRONMENT "BFC_EXE=$<TARGET_FILE:bfc>")

# Fuzz tests
add_subdirectory(fuzz)