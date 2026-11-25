# BasicForge Dependencies Registry

This document lists all direct and transitive dependencies used in the BasicForge
compiler toolchain and IDE, their versions, licenses, and integration status.

## Compiler Dependencies (C/C++)

| Dependency | Version/Commit | License | Status | Integration |
|------------|----------------|---------|--------|-------------|
| LLVM | Source snapshot ~2026-10-20 (main branch) | Apache-2.0 with LLVM Exception | **Integrated** (subprocess) | Built from source, linked via subprocess calls to clang.exe / lld-link.exe |
| LLD (LLVM linker) | Included with LLVM | Apache-2.0 with LLVM Exception | **Integrated** (subprocess) | Called via lld-link.exe subprocess |
| CMake | 3.20+ | BSD-3-Clause | Build system | CMake configuration |
| Ninja | 1.13.2 | Apache-2.0 | Build accelerator | CMake generator |

## IDE Dependencies (Rust)

| Dependency | Version | License | Status | Integration |
|------------|---------|---------|--------|-------------|
| egui | 0.29 | MIT OR Apache-2.0 | **Integrated** | Cargo dependency |
| eframe | 0.29 | MIT OR Apache-2.0 | **Integrated** | Cargo dependency |
| egui-winit | 0.29 | MIT OR Apache-2.0 | **Integrated** | Cargo dependency |
| egui-wgpu | 0.29 | MIT OR Apache-2.0 | **Integrated** | Cargo dependency |
| winit | 0.29 | Apache-2.0 | **Integrated** | Cargo dependency |
| serde | 1.0 | MIT OR Apache-2.0 | **Integrated** | Cargo dependency |
| serde_json | 1.0 | MIT OR Apache-2.0 | **Integrated** | Cargo dependency |
| anyhow | 1.0 | MIT OR Apache-2.0 | **Integrated** | Cargo dependency |

## Evaluated (Not Yet Integrated)

| Dependency | Version/Commit | License | Evaluation Status |
|------------|----------------|---------|-------------------|
| Cranelift | Source snapshot 2026-10-20 | Apache-2.0 with LLVM Exception | Evaluated, not integrated |
| AsmJit | master branch 2026-10-20 | zlib/libpng | Evaluated, not integrated |
| tree-sitter | master branch 2026-10-20 | MIT | Evaluated, not integrated |
| rowan | 0.18.0-alpha.2 | MIT OR Apache-2.0 | Evaluated, not integrated |
| salsa | 0.28.5 | MIT OR Apache-2.0 | Evaluated, not integrated |
| gimli | 0.34.0 | MIT OR Apache-2.0 | Evaluated, not integrated |
| goblin | 0.10.6 | MIT | Evaluated, not integrated |
| LIEF | main branch 2026-10-20 | Apache-2.0 | Evaluated, not integrated |

## System Dependencies

| Dependency | Version | Purpose |
|------------|---------|---------|
| Visual Studio C++ Build Tools | 2022 | MSVC-compatible C++ toolchain |
| Windows 10/11 SDK | 10.0.19041+ | Windows headers/libs |
| LLVM/Clang | ~19/20 | C++ compiler, LLVM libraries, LLD |
| Rust | 1.80+ | Rust toolchain for IDE |

## License Compatibility

All integrated components use permissive licenses:
- **Apache-2.0 with LLVM Exception** (LLVM, LLD) - compatible with BasicForge distribution
- **MIT** (egui, serde, anyhow, etc.) - permissive, compatible
- **Apache-2.0** (Cranelift, Ninja, etc.) - compatible
- **BSD-3-Clause** (CMake) - compatible

No copyleft (GPL, LGPL, MPL) components are integrated.

## Integration Method Summary

| Component | Method |
|-----------|--------|
| LLVM/LLD | Subprocess calls (clang.exe, lld-link.exe) — **needs replacement with direct library integration** |
| Rust deps | Cargo managed, statically linked into basicforge-ide.exe |
| Build tools | CMake + Ninja, invoked at build time |

## Next Steps for Dependencies

1. **P0**: Replace LLVM/LLD subprocess calls with direct C++ API integration
2. **P1**: Integrate tree-sitter, salsa, rowan for IDE language services
3. **P1**: Add gimli for DWARF debug info support
4. **P2**: Evaluate Cranelift as supplemental backend