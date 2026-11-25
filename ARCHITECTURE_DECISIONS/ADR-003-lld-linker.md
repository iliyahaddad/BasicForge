# ADR-003: LLD as the primary Windows PE/COFF linker

## Status
Accepted

## Decision
LLD (from llvm-project) is the linker for the initial production architecture.
LLD's COFF linker is used to produce PE32+ executables from LLVM-generated COFF
object files plus the statically linked BasicForge runtime.

## Components reused
- LLD COFF linker (lld/COFF)

## Integration method
- Prefer direct LLD C++ library integration into bfc.exe.
- If direct library integration is impractical, invoke lld-link.exe behind the
  compiler CLI (hidden from the user; the user runs `bfc build` only).

## Rationale
- Avoids writing a custom PE/COFF linker before understanding LLD.
- Proven Windows linking (imports, exports, relocations, section handling).
- A custom BasicForge linker may be developed incrementally later and compared
  against LLD output.

## Constraints
- The user-facing compiler provides a simple build command; all LLD details
  are hidden.
- Users never write linker scripts or handle COFF structures.

## Related
ADR-002 (LLVM backend)
