# BasicForge Third-Party Notices

This document lists the third-party open-source components reused or integrated
into the BasicForge compiler toolchain and IDE. Copyright and attribution
notices are preserved as required by each license.

## 1. LLVM / LLVM Project (including LLD)

- **Project:** LLVM Compiler Infrastructure (including LLD linker)
- **Repository:** https://github.com/llvm/llvm-project
- **Version / commit:** Source snapshot downloaded 2026-11-03 (main branch),
  installed binary at C:\Program Files\LLVM (version ~19/20, built 2026-10-20).
  Exact source commit not recorded because downloads are zip extracts without
  .git metadata.
- **License:** Apache License v2.0 with LLVM Exceptions
  (see llvm-project/LICENSE.TXT)
- **Reused source modules:** Used as external build tools (clang, lld-link).
  When the LLVM development libraries are installed, the optional LLVM IR /
  optimization / X86 code generation / Object / LLD COFF linker components are
  linked directly.
- **Integration method:** The default build uses LLVM/clang and lld-link as
  external tools: BasicForge emits C, compiles it with `clang -O2`, and links
  with `lld-link`. When a full LLVM development installation is present
  (`LLVM.lib` + headers), the optional LLVM C++ backend (`build_llvm_ir`) and
  the LLD library API are enabled via `BFC_LLVM_HEADERS_AVAILABLE`. The user
  never invokes these tools directly.
- **Modifications:** None yet (direct integration).
- **Required attribution:** LLVM Project copyright notices preserved in source.
- **Known limitations:** Large build time; X86-64 Windows target only in
  milestone 1; PDB/CodeView debug info requires separate investigation.

## 2. Cranelift (via Wasmtime)

- **Project:** Cranelift code generator
- **Repository:** https://github.com/bytecodealliance/wasmtime (cranelift/)
- **Version / commit:** Source snapshot downloaded 2026-11-03 (main branch).
- **License:** Apache License v2.0 WITH LLVM-exception
- **Reused source modules:** cranelift/codegen, cranelift/assembler-x64
  (evaluated as a potential alternative/supplemental native backend).
- **Integration method:** Evaluated only; not yet integrated into the
  compiler. LLVM is the primary backend for milestone 1.
- **Modifications:** None.
- **Required attribution:** Cranelift / Wasmtime copyright notices preserved.
- **Known limitations:** Wasmtime is WebAssembly-focused; Cranelift is a
  codegen backend, not a full linker/PE writer.

## 3. AsmJit

- **Project:** AsmJit
- **Repository:** https://github.com/asmjit/asmjit
- **Version / commit:** Source snapshot downloaded 2026-11-03 (master branch).
- **License:** Reviewed under db\LICENSE.md (permissive).
- **Reused source modules:** Machine-code generation infrastructure
  (evaluated only; compared against LLVM MC).
- **Integration method:** Evaluated only; not integrated.
- **Modifications:** None.
- **Required attribution:** AsmJit copyright notices preserved if used.
- **Known limitations:** C++ JIT/assembler library; not a linker or PE writer.

## 4. egui

- **Project:** egui (immediate mode GUI)
- **Repository:** https://github.com/emilk/egui
- **Version / commit:** Source snapshot downloaded 2026-11-03 (main branch);
  crate version 0.x (workspace).
- **License:** MIT OR Apache-2.0 (dual)
- **Reused source modules:** crates/egui, crates/epaint, crates/emath,
  crates/ecolor (candidate Rust IDE GUI framework).
- **Integration method:** Evaluated for the Rust IDE; not yet integrated.
- **Modifications:** None.
- **Required attribution:** egui copyright notices preserved.
- **Known limitations:** Immediate-mode GUI; needs a backend (winit/wgpu or
  raw-windows) for native desktop windows.

## 5. tree-sitter

- **Project:** tree-sitter (incremental parser)
- **Repository:** https://github.com/tree-sitter/tree-sitter
- **Version / commit:** Source snapshot downloaded 2026-11-03 (master branch).
- **License:** MIT
- **Reused source modules:** crates/tree-sitter (parser generation/runtime)
  (candidate for editor-oriented incremental parsing in the IDE).
- **Integration method:** Evaluated for IDE language services only; the
  compiler owns the authoritative grammar.
- **Modifications:** None.
- **Required attribution:** tree-sitter copyright notice preserved.
- **Known limitations:** Not the compiler frontend; used only for editor
  syntax trees if adopted.

## 6. rowan

- **Project:** rowan (lossless syntax tree)
- **Repository:** https://github.com/rust-analyzer/rowan
- **Version / commit:** Source snapshot downloaded 2026-11-03 (master);
  crate version 0.18.0-alpha.2.
- **License:** MIT OR Apache-2.0 (dual)
- **Reused source modules:** rowan core (candidate Rust syntax tree).
- **Integration method:** Evaluated for IDE language services; not integrated.
- **Modifications:** None.
- **Required attribution:** rowan copyright notices preserved.
- **Known limitations:** Pre-release alpha version.

## 7. salsa

- **Project:** salsa (incremental computation)
- **Repository:** https://github.com/salsa-rs/salsa
- **Version / commit:** Source snapshot downloaded 2026-11-03 (master);
  crate version 0.28.5.
- **License:** MIT OR Apache-2.0 (dual, workspace)
- **Reused source modules:** salsa core (candidate for incremental IDE
  analysis/semantic caching).
- **Integration method:** Evaluated for IDE language services; not integrated.
- **Modifications:** None.
- **Required attribution:** salsa copyright notices preserved.
- **Known limitations:** Experimental framework.

## 8. gimli

- **Project:** gimli (DWARF reader/writer)
- **Repository:** https://github.com/gimli-rs/gimli
- **Version / commit:** Source snapshot downloaded 2026-11-03 (main);
  crate version 0.34.0.
- **License:** MIT OR Apache-2.0 (dual)
- **Reused source modules:** gimli core (candidate for debug info reading).
- **Integration method:** Evaluated for IDE/debugger debug-info handling;
  not integrated.
- **Modifications:** None.
- **Required attribution:** gimli copyright notices preserved.
- **Known limitations:** DWARF-focused; Windows CodeView/PDB handled separately
  via LLVM.

## 9. goblin

- **Project:** goblin (binary parsing: ELF, Mach-O, PE, archive)
- **Repository:** https://github.com/m4b/goblin
- **Version / commit:** Source snapshot downloaded 2026-11-03 (master);
  crate version 0.10.6.
- **License:** MIT
- **Reused source modules:** goblin::pe (candidate PE parsing/inspection in
  Rust IDE/tooling).
- **Integration method:** Evaluated for PE inspection in IDE tooling; not
  integrated.
- **Modifications:** None.
- **Required attribution:** goblin copyright notices preserved.
- **Known limitations:** Parser/inspector, not a linker.

## 10. LIEF

- **Project:** LIEF (instrumentation / executable format library)
- **Repository:** https://github.com/lief-project/LIEF
- **Version / commit:** Source snapshot downloaded 2026-11-03 (main).
- **License:** Reviewed under plugins\ghidra\LICENSE (permissive).
- **Reused source modules:** PE parsing/inspection (candidate).
- **Integration method:** Evaluated for PE validation; not integrated.
- **Modifications:** None.
- **Required attribution:** LIEF copyright notices preserved.
- **Known limitations:** Primarily an analysis/instrumentation library.

## 11. Ninja

- **Project:** Ninja build system
- **Repository:** https://github.com/ninja-build/ninja
- **Version / commit:** ninja-1.13.2 (downloaded 2026-11-03).
- **License:** Apache License v2.0
- **Reused source modules:** ninja.exe build tool (build acceleration only;
  not linked into BasicForge).
- **Integration method:** Used as the CMake generator for building the
  compiler and LLVM.
- **Modifications:** None.
- **Required attribution:** Ninja copyright notice preserved.
- **Known limitations:** Build tool only; no runtime distribution impact.

## License Compatibility Summary

All listed components use permissive licenses (MIT, Apache-2.0, or Apache-2.0
with LLVM Exception). These are mutually compatible and compatible with
redistribution of BasicForge in source or binary form. No copyleft (GPL,
LGPL, MPL) components are integrated into the compiler or IDE.
