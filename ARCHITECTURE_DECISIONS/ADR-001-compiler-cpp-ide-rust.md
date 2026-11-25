# ADR-001: Compiler in C++, IDE in Rust

## Status
Accepted

## Decision
The BasicForge compiler frontend, semantic analysis, IR, optimizer, native
code generation, object/COFF handling, and runtime are implemented in C++.
The IDE application, GUI, editor integration, project management, and language
services are implemented in Rust.

## Rationale
- The mission mandates this division; C++ is the natural fit for integrating
  LLVM/LLD C++ libraries directly; Rust is the natural fit for a modern
  desktop GUI (egui, windows-rs, tower-lsp).
- Keeps the two applications independent (bfc.exe works without the IDE).
- C++/Rust boundary is minimized to a stable C ABI or LSP/IPC interface.

## Consequences
- Two separate build systems: CMake (compiler) and Cargo (IDE).
- A defined interface (LSP for language intelligence; CLI/build protocol for
  compilation) separates IDE from compiler internals.
- No tight coupling of IDE to compiler implementation details.

## Related
ADR-005 (IDE-compiler interface)
