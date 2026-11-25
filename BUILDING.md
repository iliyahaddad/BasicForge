# Building BasicForge

## Prerequisites

- CMake 3.20+
- C++20 compiler (MSVC, Clang, or GCC)
- Rust 1.70+
- LLVM 17+ (optional, for LLVM backend)
- Git

## Build Instructions

### Compiler

```bash
cd compiler
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### IDE

```bash
cd ide
cargo build --release
```

### Runtime

```bash
cd runtime
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

## Running Tests

```bash
# Compiler tests
cd compiler/build
ctest -C Release

# IDE tests
cd ide
cargo test
```

## Building with LLVM

To enable the LLVM backend:

1. Install LLVM 17+
2. Set `LLVM_DIR` to the LLVM CMake directory
3. Build with `-DBFC_WITH_LLVM=ON`

```bash
cmake -B build -DLLVM_DIR=/path/to/llvm/lib/cmake/llvm
```