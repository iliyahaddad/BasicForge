# BasicForge Architecture

This document describes the high-level architecture of the BasicForge compiler
toolchain and IDE, key design decisions, and component interactions.

> **Build note (current default):** the shipped build uses the **C emission
> backend** (`codegen.cpp` → C source → `clang -O2` → COFF → `lld-link`).
> The LLVM C++ backend described below is optional and is only compiled when
> `BFC_LLVM_HEADERS_AVAILABLE` is defined (LLVM development headers + libraries
> must be provided). The frontend, IR, optimizer and runtime code generation
> are shared by both backends.

---

## 1. System Overview

The compiler frontend and BasicForge IR are shared by two alternative code-generation routes. The C-emission route is the default; the direct LLVM C++ route is optional and requires LLVM development headers and libraries.

```text
┌────────────────────────────────────────────────────────────────────┐
│                       BasicForge Ecosystem                          │
├────────────────────────────────────────────────────────────────────┤
│  Rust IDE  ◄──────────────►  C++ Compiler                           │
│      │                         │                                     │
│      │                         ▼                                     │
│      │                 Lexer → Parser → Semantic Analysis            │
│      │                         │                                     │
│      │                         ▼                                     │
│      │                  BasicForge IR (BFIR)                         │
│      │                         │                                     │
│      │              ┌──────────┴──────────┐                          │
│      │              ▼                     ▼                          │
│      │        Default route         Optional route                   │
│      │         C emission             LLVM C++ API                   │
│      │              │                     │                          │
│      │              ▼                     ▼                          │
│      │        Generated C              LLVM IR                       │
│      │              │                     │                          │
│      │              ▼                     ▼                          │
│      │           Clang              LLVM CodeGen                     │
│      │              │                     │                          │
│      │              └──────────┬──────────┘                          │
│      │                         ▼                                     │
│      │                     LLD Linker                                │
│      │                         ▼                                     │
│      │                    PE32+ Output                               │
│      └──── Local language services / compiler LSP                    │
└────────────────────────────────────────────────────────────────────┘
```

---

## 2. Compiler Architecture (C++)

### 2.1 Pipeline Stages

The shared frontend pipeline is:

```text
Source (.bf)
    │
    ▼
Lexer → Tokens
    │
    ▼
Parser → AST
    │
    ▼
Semantic Analysis → Annotated AST + Symbol Tables
    │
    ▼
BFIR Lowering → BasicForge IR
    │
    ├────────────────────────────────┐
    ▼                                ▼
Default: C emission           Optional: LLVM C++ backend
    │                                │
    ▼                                ▼
Generated C source              LLVM IR / optimization
    │                                │
    ▼                                ▼
clang -O2 → COFF object       LLVM TargetMachine → COFF object
    │                                │
    └────────────────┬───────────────┘
                     ▼
                 LLD linker
                     │
                     ▼
              PE32+ executable
```

The two routes share the BasicForge frontend and BFIR. The optional direct LLVM route is enabled only when the required LLVM development headers and libraries are available at build time.

### 2.2 Key Components

| Component | File | Responsibility |
|-----------|------|----------------|
| Lexer | `lexer.cpp` | Tokenization, case-insensitive keywords |
| Parser | `parser.cpp` | Recursive descent, VB-style grammar |
| AST | `ast.h`/`ast.cpp` | Nodes: Module, Class, Sub/Function, Expressions, Statements |
| Semantic | `semantic.cpp` | Type checking, symbol tables, name resolution |
| Type System | `type_system.cpp` | Primitive types, arrays, generics (scaffolding) |
| BasicForge IR | `ir.h`/`ir_builder.cpp` | Language-owned IR: instructions, functions, blocks |
| Code generation | `codegen.cpp` | BFIR → generated C by default, or LLVM IR through the optional LLVM C++ backend |
| Runtime CodeGen | `runtime_codegen.cpp` | Generates C runtime (`*_runtime.c`) |
| LLD Integration | `lld_integration.cpp` | Linker invocation (currently subprocess) |

### 2.3 BasicForge IR Design

```
IrModule
  ├── Functions (vector<IrFunction>)
  │     ├── name, return_type, parameters
  │     └── BasicBlocks (vector<IrBasicBlock>)
  │           └── Instructions (vector<IrInstruction>)
  │                 ├── opcode (IrOpcode)
  │                 ├── operands (vector<IrValue>)
  │                 ├── result (IrValue)
  │                 └── label, label2 (for branches)
  └── String constants (via string pool)

IrOpcode categories:
  - Arithmetic: Add, Sub, Mul, Div, IntDiv, Mod, Negate
  - Logic: And, Or, Xor, Not
  - Comparison: Eq, Ne, Lt, Gt, Le, Ge
  - Control flow: Branch, Jump, Label, Return
  - Memory: Local, Store, Load, Alloc
  - Arrays: ArrayAlloc, ArrayStore, ArrayLoad, ArrayLen
  - Objects: ObjAlloc, FieldStore, FieldLoad, CallVirt
  - Calls: Call (user), CallBuiltin (runtime), CallVirt (virtual)
  - Strings: Concat
  - Type conversion: CInt, CLng, CDbl, CStr, CBool
```

---

## 3. IDE Architecture (Rust)

### 3.1 Component Structure

```
basicforge-ide/
├── src/
│   ├── main.rs          # Entry point, eframe setup
│   ├── app/
│   │   ├── mod.rs       # App state, eframe::App impl
│   │   └── app_impl.rs  # UI logic, tabs, menus, events
│   ├── editor/
│   │   └── mod.rs       # Code editor, syntax highlighting, completion
│   ├── language/
│   │   └── mod.rs       # Local Language Server (completion, hover, diagnostics)
│   ├── project/
│   │   └── mod.rs       # Project management (.bfproj, files)
│   ├── designer/
│   │   └── mod.rs       # Form Designer (toolbox, properties, code gen)
│   ├── project/
│   │   └── mod.rs       # Project management
│   └── ui/
│       └── mod.rs       # Output window, shared UI components
```

### 3.2 Language Intelligence (Local)

The IDE implements a **local language server** in `language/mod.rs`:

```
Editor keystrokes
      │
      ▼
┌─────────────────────────────────────────┐
│ Editor → on_change(content)             │
└─────────────────────────────────────────┘
      │
      ▼
┌─────────────────────────────────────────┐
│ LanguageServer.analyze_file(path, text) │
│   ├── Lexer → Parser → Semantic         │
│   └── Updates:                          │
│       ├── SymbolTable (completion)      │
│       ├── Diagnostics (errors/warnings) │
│       └── Hover info (symbol lookup)    │
└─────────────────────────────────────────┘
      │
      ▼
┌─────────────────────────────────────────┐
│ Editor renders:                         │
│   ├── Completion popup                  │
│   ├── Hover tooltip                     │
│   ├── Diagnostics panel                 │
│   └── Go-to-definition                  │
└─────────────────────────────────────────┘
```

**Features implemented:**
- Code completion (keywords, types, functions, variables)
- Hover tooltips (symbol info: type, location)
- Diagnostics (unclosed blocks, syntax errors)
- Go-to-definition (click on symbol)

**Planned (Tree-sitter/Salsa):**
- Incremental parsing (no full re-parse on keystroke)
- Semantic caching (Salsa database)
- LSP server for external editor support

---

## 4. Form Designer Architecture

```
FormDesigner
├── FormDesign (serializable)
│   ├── name, width, height
│   ├── controls: Vec<FormControl>
│   └── selected_control: Option<String>
│
├── Toolbox (left panel)
│   └── ControlType enum (Button, Label, TextBox, ...)
│
├── Properties Panel (right)
│   ├── Position/Size (X, Y, Width, Height)
│   └── Control-specific properties (Text, Name, etc.)
│
└── Code Generation
    └── generate_code() → BasicForge class with InitializeComponent()
```

**Features:**
- Drag-to-add from toolbox (click to add)
- Snap-to-grid positioning
- Property grid with live editing
- Code generation: `Class FormName { InitializeComponent() }`
- Serializable to JSON (for `.bfform` files)

---

## 5. Communication: IDE ↔ Compiler

### Current: Subprocess CLI

```rust
// IDE calls bfc.exe as subprocess
std::process::Command::new("bfc.exe")
    .arg("build")
    .arg(project_path)
    .output()
```

**Output parsing:**
- Stdout: `[bfc] command`, progress
- Stderr: Errors, warnings
- Exit code: 0 = success, non-zero = failure

### Planned: LSP + Build Protocol

```
IDE (LSP Client)          Compiler (LSP Server)
      │                         │
      ├── textDocument/didOpen  │
      │◄────────────────────────│
      ├── textDocument/didChange│
      │◄────────────────────────│
      ├── textDocument/completion
      │◄────────────────────────│
      ├── textDocument/hover
      │◄────────────────────────│
      ├── textDocument/definition
      │◄────────────────────────│
      │                         │
      ├── workspace/build (custom)
      │◄────────────────────────│
```

---

## 6. Runtime Architecture

### 6.1 Generated Runtime

The compiler generates a C file (`*_runtime.c`) per compilation:

```c
// String functions
char* bf_str_concat(const char* a, const char* b);
int64_t bf_len(const char* s);
char* bf_trim(const char* s);
char* bf_left(const char* s, int64_t n);
char* bf_right(const char* s, int64_t n);
char* bf_mid(const char* s, int64_t start, int64_t count);
int64_t bf_instr(const char* s, const char* sub);
char* bf_replace(const char* s, const char* old, const char* new_);

// Type conversions
int64_t bf_cint(double v);
int64_t bf_clng(double v);
double bf_cdbl(int64_t v);
char* bf_cstr(int64_t v);
int bf_cbool(int64_t v);

// I/O
void bf_print_string(const char* s);
void bf_print_int(int64_t v);
void bf_print_double(double v);
void bf_print_bool(int v);

// Entry point
int main(void) { UserMain(); return 0; }
```

### 6.2 Linking

```
User code (.obj) + Runtime (.obj) + msvcrt.lib + legacy_stdio_definitions.lib
                    │
                    ▼
             lld-link.exe
                    │
                    ▼
            PE32+ .exe (x64)
```

**Entry point:** `mainCRTStartup` → calls generated `main()` → calls user `Sub Main()`

---

## 7. Key Design Decisions

| Decision | Rationale |
|----------|-----------|
| **Own IR (BasicForge IR)** | Language independence; LLVM is backend detail |
| **Case-insensitive lexer** | VB compatibility |
| **Subprocess LLVM/LLD** | Pragmatic MVP; P0 to replace with library API |
| **Subprocess IDE↔Compiler** | Simple IPC; LSP planned for M7 |
| **Rust IDE, C++ Compiler** | Language-appropriate: GUI in Rust, compiler in C++ |
| **BasicForge IR** | Language-owned, optimizable before LLVM |
| **LLVM as library** | Target: P0 replacement of subprocess calls |
| **Form Designer in IDE** | Visual Basic experience; generates BF code |
| **LSP for IDE** | Standard protocol; enables external editors |

---

## 6. File Formats

| Extension | Description | Format |
|-----------|-------------|--------|
| `.bf` | BasicForge source | UTF-8 text |
| `.bfproj` | Project file | JSON (planned) |
| `.bfform` | Form Designer | JSON (serializable `FormDesign`) |
| `.bfres` | Resources | Binary (planned) |
| `.ll` | LLVM IR intermediate | LLVM text format |
| `.obj` | COFF object | COFF/PE |
| `.exe` | Native executable | PE32+ (x64) |

---

## 8. Future Extensibility

| Area | Extension Point |
|------|-----------------|
| **Backends** | LLVMCodeGen trait → Cranelift, AsmJit |
| **Languages** | Parser trait → other frontends |
| **IDE Editors** | LSP server → VS Code, Neovim, etc. |
| **Formats** | Serialization traits → `.bfform`, `.bfproj` |
| **Runtime** | Modular C modules → collections, I/O, etc. |

---

## 9. Security Considerations

- **No arbitrary code execution** from source (compiler is deterministic)
- **Subprocess sanitization** - arguments escaped, paths validated
- **No `eval`/`exec`** in language (statically compiled)
- **Static linking** - no external runtime DLLs to hijack
- **Input validation** - all file paths canonicalized, no traversal

---

## 10. Performance Targets

| Metric | Target |
|--------|--------|
| Compile HelloWorld | < 2s (cold), < 500ms (warm) |
| IDE startup | < 1s |
| Editor latency (keystroke → render) | < 16ms (60fps) |
| Completion popup | < 50ms |
| Executable size (HelloWorld) | < 200 KB |
| Memory (IDE) | < 200 MB |
| Memory (Compiler) | < 500 MB |
