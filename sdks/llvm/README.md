# LLVM / LLD SDK for BasicForge

BasicForge uses LLVM/clang and lld-link as its backend toolchain.

## Options

1. **System install** (recommended)
   - Install LLVM 19+ x64: `winget install LLVM.LLVM`
   - Or extract the LLVM distribution to `C:\Program Files\LLVM`.

2. **Bundled SDK** (offline)
   - Extract LLVM into `sdks/llvm/`.
   - CMake: `cmake -S compiler -B build -DBFC_LLVM_ROOT=C:/.../BasicForge/sdks/llvm`
   - Runtime: set `BFC_LLVM_BIN` to `sdks/llvm/bin`.

## Direct LLVM backend

The optional LLVM C++ backend is enabled automatically when the development
libraries are present:

- `include/llvm/IR/Module.h`
- `lib/LLVM.lib`
- `lib/LLD.lib` (for direct LLD linking)

Otherwise the compiler uses C emission + `clang -O2` + `lld-link`, which is
functionally equivalent but not a direct library integration.

## Notes

- `bfc.exe` hides all LLVM/LLD details from the user.
- `BFC_LLVM_ROOT` selects the LLVM installation (CMake).
- `BFC_LLVM_BIN` selects the directory containing `clang.exe`/`lld-link.exe`
  (runtime).