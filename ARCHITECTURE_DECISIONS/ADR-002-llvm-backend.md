# ADR-002: LLVM as the primary native backend

## Status
Accepted

## Decision
LLVM (from llvm-project) is integrated directly as the native code-generation
and optimization backend for the BasicForge compiler. BasicForge owns its own
AST, language semantics, type system, and a dedicated BasicForge IR. A
BasicForge-IR-to-LLVM-IR translation layer lowers BasicForge IR to LLVM IR,
after which LLVM's X86 target, CodeGen, MC, and optimization passes produce
native x64 machine code and COFF objects.

## Components reused
- LLVM Support, ADT
- LLVM IR (IRReader, AsmParser, IR, Core)
- LLVM Optimization passes
- LLVM CodeGen (X86 target, instruction selection, register allocation)
- LLVM MC (instruction encoding, object emission)
- LLVM Object (COFF reading/writing)

## Rationale
- Avoids reinventing a register allocator, instruction selector, x64 encoder,
  optimizer, and COFF writer — all mature, production-quality problems solved
  by LLVM.
- Provides a working Windows x64 PE32+ path quickly (milestone 2).
- Backend interface remains modular/replaceable; a custom BasicForge backend
  may be considered in a future milestone only if justified by benchmarks.

## Constraints
- LLVM must NOT dictate BasicForge language syntax or user-facing identity.
- BasicForge AST, semantics, and compiler interfaces remain independent of LLVM.
- Users never see LLVM IR, assembly, or machine code.

## Alternatives considered
- Cranelift (evaluated; WebAssembly-focused, not a full PE/linker solution;
  kept as documented alternative).
- AsmJit (evaluated; JIT assembler, not a linker/optimizer; not selected).
- Custom x64 backend (rejected for milestone 1; too much reinvention).

## Related
ADR-003 (LLD linker), ADR-004 (BasicForge IR)
