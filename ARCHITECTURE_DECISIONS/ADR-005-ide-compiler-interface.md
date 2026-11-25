# ADR-005: IDE-compiler interface (LSP + CLI)

## Status
Accepted

## Decision
The Rust IDE communicates with the C++ compiler (bfc.exe) through two
separate, well-defined channels:

1. Language Server Protocol (LSP) — via tower-lsp — for language intelligence
   (diagnostics, completion, navigation, hover, semantic tokens).
2. A build protocol / CLI interface — `bfc build`, `bfc check`, `bfc run`,
   `bfc clean`, `bfc new`, `bfc fmt`, `bfc test` — for compilation operations.

The IDE launches bfc.exe as a subprocess and speaks LSP over stdio (or a
named pipe/socket). The IDE does not link against or import compiler internals.

## Rationale
- Keeps IDE and compiler fully independent applications.
- LSP is a mature, standard protocol for language intelligence.
- CLI is simple, scriptable, and testable without the IDE.
- Minimizes the C++/Rust boundary to text-based messages.

## Related
ADR-001 (compiler C++, IDE Rust)
