# ADR-004: BasicForge IR + LLVM IR translation layer

## Status
Accepted

## Decision
BasicForge defines its own intermediate representation (BasicForge IR),
independent of LLVM. The compiler pipeline is:

  BasicForge source -> lexer -> parser -> AST -> semantic analysis/type check
  -> BasicForge IR -> optimization (BasicForge-level) -> LLVM IR translation
  -> LLVM optimization/codegen -> COFF object -> LLD link -> PE32+ EXE

A dedicated translation layer lowers BasicForge IR to LLVM IR. LLVM is used
only below this layer.

## Rationale
- BasicForge owns its language semantics and IR; LLVM is an implementation
  detail of the backend, not the language definition.
- Keeps the backend replaceable (ADR-002).
- Allows BasicForge-level optimizations before lowering to LLVM.

## Related
ADR-002 (LLVM backend)
