# BasicForge Language Design Decisions

**Version:** 0.1

This document records the language-level design decisions for BasicForge,
separate from the compiler/IDE architecture decisions in `ARCHITECTURE_DECISIONS/`.

## LD-001: VB6 as primary syntax reference

**Decision:** BasicForge syntax follows Visual Basic 6 block structure
(`If/Then/Else/End If`, `Select Case`, `For/To/Next`, `While/End While`,
`Sub`/`End Sub`, `Function/End Function`, `Module/End Module`).

**Rationale:** The project vision explicitly targets VB6 readability. VB6's
English-like keywords and explicit terminators are well understood by the target
audience.

**Consequence:** No C-style braces, semicolons, or `return` statements as the
primary idiom. `Return` is used but `End X` terminators are mandatory.

## LD-002: Case-insensitive language

**Decision:** Keywords and identifiers are case-insensitive.

**Rationale:** VB6 and VB.NET are both case-insensitive. This is a core VB
identity requirement.

**Consequence:** The lexer must normalize keyword comparison and identifier
lookup to a single case. The current Milestone 2 lexer is case-sensitive and
must be fixed (see audit).

## LD-003: Strong static typing, no Variant

**Decision:** Every value has a static type. There is no `Variant` type and no
implicit dynamic dispatch.

**Rationale:** A strongly typed language enables static analysis, optimization,
and native code generation without runtime type checks. VB6's `Variant` is
omitted deliberately.

**Consequence:** Type mismatches are compile errors. Explicit conversion
functions (`CInt`, `CStr`, etc.) are planned for narrowing.

## LD-004: Type inference for local variables

**Decision:** `Dim x = initializer` infers the variable's type from the
initializer (VB.NET-style).

**Rationale:** Reduces verbosity for obvious types while keeping static typing.

**Consequence:** The initializer is required when `As type` is omitted. The
type is fixed at the point of declaration.

## LD-005: Two-token block terminators

**Decision:** Blocks close with `End <keyword>` (two tokens), e.g. `End If`,
`End Sub`, `End While`, `End Select`, `End Module`. Single-token forms like
`EndIf` are not used.

**Rationale:** Matches VB6/VB.NET exactly and avoids tokenization ambiguity.

**Consequence:** The lexer must not produce `EndIf`/`EndSub`/etc. as single
tokens. The current Milestone 2 lexer has these and must be changed.

## LD-006: `Print` as the initial builtin

**Decision:** `Print(expr, ...)` is the sole builtin output routine initially.

**Rationale:** Provides immediate feedback for HelloWorld-style programs while
the standard library is developed.

**Consequence:** `Print` is treated as a special builtin call in the compiler;
it is not a user-overridable procedure initially.

## LD-007: Exactly one `Sub Main` entry point

**Decision:** Every executable program must define exactly one parameterless
`Sub Main()` with no return value. Execution starts there.

**Rationale:** Simple, predictable program startup; matches VB `Sub Main`
convention.

**Consequence:** The compiler must error if zero or multiple `Sub Main`
definitions exist across modules.

## LD-008: No implicit narrowing

**Decision:** Widening conversions (Integer→Long→Double→String) are implicit;
narrowing requires an explicit conversion function.

**Rationale:** Prevents silent data loss while keeping common cases convenient.

**Consequence:** Conversion functions `CInt`, `CLng`, `CDbl`, `CStr`, `CBool`
are planned but not yet implemented.

## LD-009: `/` is floating-point division; `\` is integer division

**Decision:** `/` always returns `Double` (VB semantics). `\` performs integer
division truncating toward zero.

**Rationale:** Matches VB6/VB.NET exactly and avoids the C `/` ambiguity.

**Consequence:** Codegen must emit different machine instructions for `/` and
`\`.

## LD-010: `&` is string concatenation only

**Decision:** `&` concatenates strings. It is not used for address-of or
bitwise AND.

**Rationale:** Matches VB. `And` is the logical/bitwise AND operator.

**Consequence:** Non-string operands to `&` require implicit conversion to
String (planned).

## LD-011: BasicForge owns its grammar; LLVM is a backend only

**Decision:** The language grammar, AST, and semantics are defined
independently of LLVM. LLVM consumes BasicForge IR below a translation layer.

**Rationale:** Prevents LLVM concepts from leaking into the user-facing
language; keeps the backend replaceable.

**Consequence:** No LLVM IR types, instructions, or concepts appear in language
docs or error messages shown to users.

## LD-012: Planned features explicitly deferred

**Decision:** Arrays, classes/properties, `For Each`, `Do/Loop`, `Try/Catch`,
enums, structures, access modifiers, and the standard library are explicitly
deferred to Milestone 4+.

**Rationale:** Avoid scope creep before the core language (variables, types,
arithmetic, conditions, loops, sub/functions, print) compiles and runs.

**Consequence:** These must not be advertised as supported in README or
marketing until implemented and tested.
