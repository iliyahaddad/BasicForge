# ADR-006: Initial platform — Windows 10/11 x64, PE32+

## Status
Accepted

## Decision
The initial milestone targets Windows 10 and Windows 11 on AMD64/x86-64,
producing PE32+ executables using the Windows x64 calling convention. Linux,
macOS, and ARM64 are explicitly out of scope for milestone 1, though the
architecture is designed to be extensible.

## Rationale
- Focuses the first working executable on a single, well-understood platform.
- LLVM X86 target and LLD COFF linker provide mature Windows x64 support.
- Avoids scope creep that would delay the first executable.

## Extensibility
- Backend and linker interfaces are modular (ADR-002, ADR-003).
- Future targets can be added by extending the BasicForge IR translation and
  backend selection without changing the frontend.
