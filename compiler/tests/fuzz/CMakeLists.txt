# Fuzz tests for BasicForge
cmake_minimum_required(VERSION 3.20)

# Standalone fuzz runner
add_executable(fuzz_runner
    fuzz_runner.cpp
)
target_link_libraries(fuzz_runner PRIVATE bfc_frontend)
target_include_directories(fuzz_runner PRIVATE ${CMAKE_SOURCE_DIR}/include)

# LibFuzzer targets (when using clang with -fsanitize=fuzzer)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_executable(lexer_fuzzer
        lexer_fuzzer.cpp
    )
    target_link_libraries(lexer_fuzzer PRIVATE bfc_frontend)
    target_include_directories(lexer_fuzzer PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_compile_options(lexer_fuzzer PRIVATE -fsanitize=fuzzer,address,undefined)
    
    add_executable(parser_fuzzer
        parser_fuzzer.cpp
    )
    target_link_libraries(parser_fuzzer PRIVATE bfc_frontend)
    target_include_directories(parser_fuzzer PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_compile_options(parser_fuzzer PRIVATE -fsanitize=fuzzer,address,undefined)
endif()

# Add fuzz tests to CTest
add_test(NAME fuzz_lexer COMMAND fuzz_runner lexer 100 100)
add_test(NAME fuzz_parser COMMAND fuzz_runner parser 100 100)
add_test(NAME fuzz_both COMMAND fuzz_runner both 50 200)