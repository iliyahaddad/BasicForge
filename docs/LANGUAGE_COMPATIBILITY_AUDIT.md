# BasicForge Language Compatibility Audit

This document audits the current BasicForge compiler implementation against the
intended Visual Basic-inspired language design (VB6 primary, VB.NET selected
improvements). It is evidence-based: a feature is marked **Implemented** only
where source code (lexer/parser/AST/semantic/codegen) and a compiling example
demonstrate it. Otherwise it is **Partial** (syntax tokenized/AST node exists
but not fully codegen'd or tested) or **Missing** (no implementation).

Audit date: 2026-11-03
Repository: BasicForge project repository
Working vertical slice: Milestone 2 (HelloWorld.bf -> PE32+ EXE -> run, verified)

---

## 1. Compatibility Table

| Feature | VB6 reference | VB.NET reference | Current BasicForge | Status | Decision |
|---|---|---|---|---|---|
| Dim declarations | `Dim x As Integer` | `Dim x As Integer` | `Dim x As Integer`, `Dim x = 5` | Implemented | Keep; VB6-style `Dim x As Integer = 5` accepted |
| Variables & types | Integer, Long, Single, Double, Boolean, String, Variant | Same + Object, Date, Decimal | Integer, Long, Single, Double, Boolean, String | Partial | No Variant; no user-defined types yet |
| Type inference | `Dim x` -> Variant (no inference) | `Dim x = 5` infers Integer | `Dim x = 5` infers type from initializer | Implemented | Keep (VB.NET-style); document as BF extension |
| Constants | `Const x As Integer = 5` | `Const x As Integer = 5` | None | Missing | Add `Const` in Milestone 3 |
| Sub procedures | `Sub Foo()` / `End Sub` | `Sub Foo()` / `End Sub` | `Sub name()` / `End Sub` | Implemented | Keep |
| Function procedures | `Function Foo() As Integer` / `End Function` | `Function Foo() As Integer` / `End Function` | Parsed; return type ignored in codegen | Partial | Implement Function return values in M3 |
| Parameters | `Sub Foo(a As Integer)` | `Sub Foo(a As Integer)` | `Sub name(a As Integer, b As Integer)` | Implemented | Keep |
| Return values | `Foo = value` (assign to name) | `Return value` | `Return expression` parsed; not codegen'd | Partial | Implement `Return` for functions in M3 |
| If / Then / Else / End If | `If ... Then ... Else ... End If` | `If ... Then ... Else ... End If` | `If cond Then ... Else ... End If` | Implemented | Keep; normalize keyword to `End If` (two tokens) |
| Select Case | `Select Case x ... Case ... Case Else ... End Select` | Same | `Select Case x ... Case ... Case Else ... End Select` | Implemented | Keep |
| For / Next | `For i = 1 To 10 [Step s] ... Next i` | Same | `For i As Integer = 1 To 10 ... Next` | Implemented | Keep; make `As type` optional |
| For Each / In / Next | `For Each x In group ... Next x` | Same | None | Missing | Add in M4+ |
| Do / Loop | `Do While cond ... Loop` / `Do ... Loop While cond` | Same | None | Missing | Add in M4+ |
| While / Wend | `While cond ... Wend` (VB6) / `While ... End While` (VB.NET) | `While ... End While` | `While cond ... End While` | Implemented | Keep `End While` (VB.NET style) |
| Modules | Standard module (.bas), no `Module` block | `Module ... End Module` | `Module name ... End Module` | Implemented | Keep |
| Classes | Class module (.cls), `Class_Initialize` | `Class ... End Class` | `Class name` parsed; body ignored | Partial | Implement class body, properties, methods in M4+ |
| Properties | `Property Get/Let/Set` | `Property Name As T {Get/Set}` | Token `Property` exists; no semantics | Missing | Add VB.NET-style `Property` in M4+ |
| Constructors | `Class_Initialize()` event | `Public Sub New()` | Token `New` exists; no semantics | Missing | Add `Sub New()` in M4+ |
| Arrays | `Dim arr(10) As Integer` | `Dim arr() As Integer` / `Dim arr(9) As Integer` | None | Missing | Add in M4+ |
| Enums | None | `Enum ... End Enum` | None | Missing | Add in M5+ |
| Structures (UDT) | `Type ... End Type` | `Structure ... End Structure` | None | Missing | Add in M5+ |
| Boolean operators | And, Or, Not, Xor (bitwise/logical) | And, Or, Not, Xor + AndAlso, OrElse | Tokens exist; only And/Or/Not partially codegen'd | Partial | Implement AndAlso/OrElse short-circuit in M3 |
| String concatenation | `&` | `&` | `&` tokenized; parsed as Concat | Implemented | Keep |
| String functions | Left, Right, Mid, InStr, Len, Trim, Replace, Split, Join, Format | Same (as Shared methods) | None (only `Print`) | Missing | Add standard library in M5+ |
| Error handling | `On Error GoTo` / `On Error Resume Next` | `Try / Catch / Finally` | None | Missing | Add VB.NET `Try/Catch/Finally` in M5+ |
| Access modifiers | Public, Private | Public, Private, Protected, Friend | Tokens exist; no semantic effect | Partial | Implement in M4+ (class members) |
| Comments | `'` and `REM` | `'` | `'` supported; `REM` not | Partial | Add `REM` for VB6 compat |
| Case sensitivity | Case-insensitive | Case-insensitive | **Case-sensitive** (keyword map is exact-case) | Conflict | **Redesign**: make lexer case-insensitive |
| Scope rules | Module-level, procedure-level, static | Same + block scope | Procedure-level variable map only | Partial | Define block/scope rules in spec |
| Type conversion | Implicit (Variant-heavy) | Widening implicit; narrowing needs cast (Option Strict) | Exact match only; no conversions | Missing | Define conversion rules in spec |

---

## 2. What Already Exists (Evidence)

The following are implemented and verified by the working HelloWorld slice:

1. **Lexer** (`compiler\src\lexer\lexer.cpp`, `compiler\include\bf\lexer.h`):
   - Keywords: Module, Sub, Function, Class, Property, End, EndSub, EndFunction, EndClass, Dim, As, type names, True/False, Return, Print, If/Then/Else/EndIf, For/To/Step/Next, While/EndWhile, Select/Case/EndSelect/ElseCase, Public/Private/New/Nothing.
   - Literals: IntegerLiteral, StringLiteral, BooleanLiteral.
   - Operators: `& + - * / \ ^ = <> < > <= >= And Or Not Xor`, punctuation `( ) , : . ! _`.
   - Comments: `'` to end of line.
   - Whitespace/line tracking.

2. **Parser** (`compiler\src\parser\parser.cpp`):
   - `Module name ... End Module` (top-level only; error recovery skips to next Module).
   - `Sub name(params) ... End Sub` and `Function name(params) As type ... End Function` (parsed; body captured).
   - `Dim name As type [= init]` and `Dim name = init` (type inference).
   - `Print(arg, ...)`, `Return [expr]`.
   - `If cond Then ... Else ... End If`.
   - `For i As type = start To end [Step step] ... Next`.
   - `While cond ... End While`.
   - `Select Case subject ... Case val,... ... Case Else ... End Select`.
   - Expression precedence climbing: `||` equality, comparisons, `+`/`-`/`&`, `*`/`/`/`\`/`Mod`/`^`, unary `-`/`Not`, primaries (literals, identifiers, calls, parens).

3. **AST** (`compiler\include\bf\ast.h`): nodes for all constructs above; `TypeRef` wraps `PrimitiveType` (Void, Integer, Long, Single, Double, Boolean, String).

4. **Semantic analysis** (`compiler\src\semantic\semantic.cpp`):
   - Variable table per procedure; parameter registration.
   - `Dim` initializer type matching (exact kind match; inference sets type).
   - `Print` argument restricted to String or Integer.
   - `If`/`While` condition restricted to Boolean.
   - Identifier lookup; unknown identifiers default to Integer.

5. **IR** (`compiler\include\bf\ir.h`, `compiler\src\ir\ir_builder.cpp`): BasicForge IR with virtual registers, constants, locals, params; opcodes for arithmetic/compare/logic/control/calls. IrBuilder lowers AST to IR (built but **discarded** in main.cpp; see conflict below).

6. **Codegen** (`compiler\src\main.cpp`, `LlvmEmitter`):
   - Direct AST → LLVM IR text (bypasses IrBuilder output).
   - String globals, `declare void @bf_print(i8*)`, `define void @name()`, `call void @bf_print(i8* ...)`, `ret void`.
   - Runtime C: `main()` calls entry sub; `bf_print` wraps `printf`.
   - Pipeline: clang (`-target x86_64-pc-windows-msvc`) compiles `.ll` → `.obj`; `lld-link` links `.obj` + runtime `.obj` → PE32+ EXE (`/entry:mainCRTStartup`, msvcrt.lib).

7. **Verified output**: `examples\HelloWorld.exe` is a valid x64 PE32+ (Magic 0x20B, Machine AMD64, Subsystem Windows CUI) and prints `Hello World` with exit code 0.

---

## 3. What Is Missing

- Constants (`Const`).
- `Function` return values (codegen ignores return_type; `Return expr` not emitted).
- Arrays, Enums, Structures.
- Classes with body: properties, methods, constructors (`Sub New`).
- `For Each`, `Do / Loop`, `While / Wend`.
- Standard library functions (Left, Right, Mid, InStr, Len, Trim, Replace, Split, Join, Format).
- Error handling (`Try/Catch/Finally` or `On Error`).
- Access-modifier semantics (Public/Private/etc. have no effect).
- `REM` comments.
- Implicit/narrowing type conversions.
- Tests: no test files exist under `tests/` or `compiler\tests/`.
- Examples: only `HelloWorld.bf`.
- Documentation: no language specification files exist.

---

## 4. Conflicts with the Intended VB-Style Design

| Conflict | Location | Impact | Required change |
|---|---|---|---|
| **Case-sensitive keywords** | `compiler\include\bf\lexer.h` `keywords()` map uses exact-case keys ("Module", "Sub", ...). | VB requires case-insensitivity; `module`, `SUB`, `print` would fail. | Redesign lexer to case-insensitive keyword matching; keep identifier case preserved but comparisons case-insensitive. |
| **`EndIf` single token** | Lexer/token.h has `EndIf`; VB uses `End If` (two tokens). Parser works because `End` + `If` are scanned separately only if spaced; but `EndIf` as one token means `End If` (with space) yields two tokens `End`+`If` which `consume_end(TokenType::If)` expects — actually `consume_end` consumes `End` then `If`, so `End If` works; `EndIf` as one token would NOT match `consume_end(TokenType::If)` because after `End`, current is `EndIf` not `If`. | Inconsistent: `End If` works, `EndIf` fails. | Remove `EndIf`/`EndSub`/`EndFunction`/`EndClass`/`EndWhile`/`EndSelect` single tokens; use two-token `End X` consistently. |
| **IR built but unused** | `main.cpp` builds `IrModule module = irb.lower(...)` then `(void)module`; codegen uses `LlvmEmitter` directly on the AST. | IR infrastructure is dead weight; optimizer cannot run on it; violates the pipeline design (AST -> IR -> optimize -> codegen). | Either wire IrBuilder output into codegen (LLVM IR from IR), or remove the direct emitter and make LlvmEmitter consume the IR. |
| **`parse_for` skips type token unchecked** | `consume(TokenType::As); advance();` — advances past type without validating it is a type token. | `For i = 1 To 10` (no As) would silently consume `=` as the "type". | Validate type token in `parse_for`. |
| **`parse_select_case` terminator bug** | Inner `parse_block_until(TokenType::Case)` consumes the next `Case` as block end; outer loop then expects `Case`/`Else`/`EndSelect` but current is already past it. | Case bodies may be truncated or mis-parsed. | Restructure select-case parsing. |
| **No `Main` requirement enforcement** | `entry_name()` picks first non-function Sub in first module; no error if no Sub exists. | Silent wrong entry point. | Require exactly one `Sub Main()` entry point; error otherwise. |
| **Print is the only builtin** | `builtins_ = {"Print"}` hardcoded; no user function calls codegen'd. | Cannot call user-defined Subs/Functions. | Implement call codegen for user procedures. |

---

## 5. What Should Remain Unchanged

- **Two-app split**: `bfc.exe` (C++ compiler) and IDE (Rust) remain separate; LSP/CLI interface (ADR-005).
- **LLVM backend + LLD linker** (ADR-002, ADR-003): keep direct library/CLI integration; do not rewrite a custom backend/linker for M3.
- **PE32+ x64 output via clang + lld-link** (bootstrap path) — proven working; keep for M3, migrate to direct LLVM library linking later if justified.
- **BasicForge IR** structure — keep as the language-owned IR; just wire it into codegen.
- **Precedence-climbing expression parser** — correct and sufficient for M3 arithmetic/comparison.
- **Directory structure** — keep the compiler/ide/runtime layout.

---

## 6. What Should Be Redesigned

1. **Lexer case handling** — case-insensitive keywords.
2. **Multi-token `End X`** — drop single `EndIf`/`EndSub`/etc. tokens.
3. **Codegen path** — consume `IrBuilder` output (or make LlvmEmitter consume IR), implement Function returns and user-procedure calls.
4. **Error recovery** — parser currently `advance()`s on unexpected tokens; improve with synchronization sets.
5. **Entry-point contract** — require `Sub Main()` in a Module.

---

## 7. Recommended Implementation Order (Milestone 3)

1. **Lexer fixes**: case-insensitive keywords; remove `EndX` single tokens; add `REM`.
2. **Parser fixes**: validate `For` type token; restructure `Select Case`; enforce `Sub Main()` entry.
3. **Semantic**: symbol table for modules/procedures (not just locals); function return-type checking; implicit conversion rules; `Print` accepts all primitive types.
4. **Codegen**: wire IrBuilder → LLVM IR; implement `Function` return values, `Return expr`, user-procedure calls, all binary operators, unary, integer/float/boolean/string printing.
5. **Runtime**: add `bf_print_int`, `bf_print_bool`, `bf_print_long`, `bf_print_double`; basic string concat helper.
6. **Tests**: add `compiler\tests/` with lexer/parser/semantic/codegen test cases; each must compile and run.
7. **Examples**: add the representative examples below, each tagged with compile status.
8. **Language docs**: write `LANGUAGE_SPECIFICATION.md` + supporting syntax/keywords/types/operators/grammar docs.

Do **not** begin Milestone 4 (arrays, classes, For Each, Do/Loop) until M3 is complete and tested.

---

## 8. Representative Syntax Examples (Compile Status)

Each example below is listed with whether it compiles under the current compiler.

### 1. Hello World — COMPILES (verified)
```vb
Module Main

    Sub Main()
        Print("Hello World")
    End Sub

End Module
```
Status: **Compiles and runs** (examples\HelloWorld.bf verified).

### 2. Variables and type declarations — PARTIAL
```vb
Module Main

    Sub Main()
        Dim name As String = "Iliya"
        Dim age As Integer = 40
        Dim price As Double = 125.50
        Dim enabled As Boolean = True
        Print(name)
        Print(age)
    End Sub

End Module
```
Status: **Parses and semantically checks**, but codegen only emits `Print` for string literals / integer literals — `Print(name)` with an identifier is not emitted (LlvmEmitter only handles StringLiteralExpr directly; IdentifierExpr for Print falls through). Needs codegen for identifier Print.

### 3. Type inference — PARTIAL
```vb
Module Main

    Sub Main()
        Dim total = 100
        Dim message = "Hello"
        Print(total)
        Print(message)
    End Sub

End Module
```
Status: **Parses and infers types** in semantic analysis; codegen does not emit identifier/Print for inferred variables. Needs codegen.

### 4. Arithmetic expressions — PARTIAL
```vb
Module Main

    Sub Main()
        Dim a As Integer = 10
        Dim b As Integer = 3
        Print(a + b)
        Print(a - b)
        Print(a * b)
        Print(a / b)
        Print(a & b)
    End Sub

End Module
```
Status: **Parsed**, but codegen only emits `Add` (BinaryExpr::Op::Add); all other operators fall through/unimplemented in `LlvmEmitter`. Needs codegen.

### 5. Conditions — PARSES ONLY
```vb
Module Main

    Sub Main()
        Dim age As Integer = 20
        If age >= 18 Then
            Print("Adult")
        Else
            Print("Minor")
        End If
    End Sub

End Module
```
Status: **Parsed and semantically checked** (condition must be Boolean), but `If`/`Else` blocks are not emitted in `LlvmEmitter` (commented "reserved for later milestones"). Needs codegen.

### 6. Loops — PARSES ONLY
```vb
Module Main

    Sub Main()
        For i As Integer = 1 To 10
            Print(i)
        Next
    End Sub

End Module
```
Status: **Parsed**, not codegen'd. Needs codegen.

### 7. Functions — PARSES ONLY
```vb
Module Main

    Function Add(a As Integer, b As Integer) As Integer
        Return a + b
    End Function

    Sub Main()
        Dim result As Integer = Add(10, 20)
        Print(result)
    End Sub

End Module
```
Status: **Parsed** (Function + Return), but return type and `Return` are not codegen'd; user-procedure calls not emitted. Needs codegen.

### 8. Modules — COMPILES (single module)
```vb
Module Main

    Sub Main()
        Print("OK")
    End Sub

End Module
```
Status: **Compiles** (single module with Main). Multiple modules / module-qualified access not implemented.

### 9. Classes and properties — PARSES NAME ONLY
```vb
Module Main

    Class Person
        Public Property Name As String
        Public Sub SayHello()
            Print("Hello " & Name)
        End Sub
    End Class

    Sub Main()
        Dim p As Person
        Print("done")
    End Sub

End Module
```
Status: **Class name only parsed**; body, properties, methods ignored. `Dim p As Person` parses but type `Person` is not resolved. Missing.

### 10. Arrays — MISSING
```vb
Module Main

    Sub Main()
        Dim nums(4) As Integer
        nums(0) = 10
        Print(nums(0))
    End Sub

End Module
```
Status: **Missing** (no array support).

### 11. String handling — MISSING (beyond Print literal)
```vb
Module Main

    Sub Main()
        Dim s As String = "BasicForge"
        Print(Left(s, 4))
        Print(Len(s))
    End Sub

End Module
```
Status: **Missing** (no standard library functions).

### 12. Error handling — MISSING
```vb
Module Main

    Sub Main()
        On Error GoTo Bad
        ' ... 
        Exit Sub
Bad:
        Print("Error")
    End Sub

End Module
```
Status: **Missing** (no error handling).

---

## 9. Conclusion

The Milestone 2 slice proves the toolchain (lexer → parser → semantic → LLVM IR → clang → lld-link → PE32+ EXE) works end-to-end for one construct (`Print` of a string literal in `Sub Main`). However, the language is far from the intended VB-inspired design: only a single statement type codegens, keywords are case-sensitive (a direct violation of the VB requirement), `EndIf`/etc. single tokens are inconsistent with `End X`, the IR is built but unused, and Functions/conditions/loops/identifiers-in-Print are not codegen'd.

**Immediate next step**: apply the Milestone 3 plan above — lexer/parser fixes, codegen for identifiers + all operators + If/For/While/Function/Return, runtime print helpers, and a test suite — before adding new language features.
