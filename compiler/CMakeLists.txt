cmake_minimum_required(VERSION 3.20)
project(BasicForgeCompiler VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(BASICFORGE_SDK_DIR "${CMAKE_SOURCE_DIR}/../sdks" CACHE PATH "BasicForge SDK directory")
set(BASICFORGE_RUNTIME_DIR "${CMAKE_SOURCE_DIR}/../runtime" CACHE PATH "BasicForge runtime source")
set(BFC_LLVM_ROOT "C:/Program Files/LLVM" CACHE PATH "LLVM installation root")

option(BFC_WITH_LLVM "Enable LLVM backend integration" ON)
option(BFC_BUILD_TESTS "Build compiler tests (Lit framework)" OFF)
option(BFC_BUILD_BENCHMARKS "Build benchmarks" OFF)

if(MSVC OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(/W4 /WX- /GR- /EHs-c- /GS- /O2 /Ob2)
    add_link_options(/NXCOMPAT /DYNAMICBASE)
endif()

# LLVM detection
set(LLVM_AVAILABLE OFF)
set(LLVM_C_AVAILABLE OFF)
set(LLVM_HEADERS_AVAILABLE OFF)
if(BFC_WITH_LLVM)
    if(EXISTS "${BFC_LLVM_ROOT}/include/llvm-c/Core.h")
        set(LLVM_C_AVAILABLE ON)
        set(LLVM_AVAILABLE ON)
        message(STATUS "LLVM C API found at ${BFC_LLVM_ROOT}")
    elseif(EXISTS "${BASICFORGE_SDK_DIR}/llvm/include/llvm-c/Core.h")
        set(LLVM_C_AVAILABLE ON)
        set(LLVM_AVAILABLE ON)
        message(STATUS "LLVM C API found at ${BASICFORGE_SDK_DIR}/llvm")
    else()
        message(STATUS "LLVM C API not found; building with C emission fallback")
    endif()

    # Direct LLVM C++ backend requires headers + import libraries. When a full
    # LLVM development installation is present, enable build_llvm_ir().
    if(EXISTS "${BFC_LLVM_ROOT}/include/llvm/IR/Module.h"
       AND EXISTS "${BFC_LLVM_ROOT}/lib/LLVM.lib")
        set(LLVM_HEADERS_AVAILABLE ON)
        set(LLVM_AVAILABLE ON)
        message(STATUS "LLVM C++ headers/libs found; direct LLVM backend enabled")
    endif()
endif()

# Frontend library
add_library(bfc_frontend STATIC
    src/lexer/lexer.cpp
    src/lexer/token.cpp
    src/parser/parser.cpp
    src/ast/ast.cpp
    src/semantic/semantic.cpp
    src/types/type_system.cpp
    src/ir/basicforge_ir.cpp
    src/ir/ir_builder.cpp
    src/diagnostics/diagnostic.cpp
    src/diagnostics/source_location.cpp
    src/linker/lld_integration.cpp
    src/optimizer/optimizer.cpp
    src/lsp_server.cpp
)

target_include_directories(bfc_frontend PUBLIC include)
if(MSVC)
    target_compile_options(bfc_frontend PRIVATE /wd4251 /wd4275)
endif()

# Backend library
add_library(bfc_backend STATIC
    src/backend/codegen.cpp
    src/backend/runtime_codegen.cpp
)
target_include_directories(bfc_backend PRIVATE
    ${BASICFORGE_RUNTIME_DIR}/include
    ${CMAKE_SOURCE_DIR}/include
)
if(LLVM_C_AVAILABLE)
    target_include_directories(bfc_backend PRIVATE "${BFC_LLVM_ROOT}/include")
    target_link_libraries(bfc_backend PRIVATE "${BFC_LLVM_ROOT}/lib/LLVM-C.lib")
    target_compile_definitions(bfc_backend PRIVATE BFC_LLVM_C_AVAILABLE)
    message(STATUS "Building with LLVM C API support")
endif()
if(LLVM_HEADERS_AVAILABLE)
    target_include_directories(bfc_backend PRIVATE "${BFC_LLVM_ROOT}/include")
    target_link_libraries(bfc_backend PRIVATE "${BFC_LLVM_ROOT}/lib/LLVM.lib")
    if(EXISTS "${BFC_LLVM_ROOT}/lib/LLD.lib")
        target_link_libraries(bfc_backend PRIVATE "${BFC_LLVM_ROOT}/lib/LLD.lib")
    endif()
    target_compile_definitions(bfc_backend PRIVATE BFC_LLVM_HEADERS_AVAILABLE)
    message(STATUS "Building with direct LLVM/LLD support")
endif()
if(NOT LLVM_C_AVAILABLE AND NOT LLVM_HEADERS_AVAILABLE)
    target_compile_definitions(bfc_backend PRIVATE BFC_WITH_LLVM)
    message(STATUS "Building C emission fallback backend")
endif()

# Main executable
add_executable(bfc src/main.cpp src/debugger/debugger.cpp src/debugger/dap_server.cpp)
target_link_libraries(bfc PRIVATE bfc_frontend bfc_backend)
if(MSVC)
    target_link_libraries(bfc PRIVATE dbghelp.lib)
endif()
target_include_directories(bfc PRIVATE include)
if(MSVC)
    target_include_directories(bfc PRIVATE
        "C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0/shared"
        "C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0/um"
        "C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0/ucrt"
    )
    target_compile_options(bfc PRIVATE /wd4251 /wd4275)
endif()
set_target_properties(bfc PROPERTIES OUTPUT_NAME bfc DEBUG_POSTFIX "")

install(TARGETS bfc RUNTIME DESTINATION bin)
install(DIRECTORY include/ DESTINATION include/bf)

if(BFC_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()