# BasicForge Compiler Test Suite

This directory contains lit-style tests for the BasicForge compiler.

## Test Organization

```
tests/
├── lit.cfg.py              # Lit configuration
├── lexer/                  # Lexer tests
│   ├── keywords.test       # Keyword tokenization
│   ├── identifiers.test    # Identifier handling
│   ├── literals.test       # String, number, boolean literals
│   └── operators.test      # Operator tokenization
├── parser/                 # Parser tests
│   ├── declarations.test   # Dim, Class, Module, Interface
│   ├── statements.test     # If, For, While, Select, Try
│   ├── expressions.test    # Binary, unary, calls, array access
│   └── generics.test       # Of T syntax
├── semantic/               # Semantic analysis tests
│   ├── type_checking.test  # Type inference, compatibility
│   ├── name_resolution.test# Variable/function lookup
│   └── generics.test       # Generic constraints
├── codegen/                # Code generation tests
│   ├── hello.test          # Hello World
│   ├── arithmetic.test     # Arithmetic operations
│   ├── control_flow.test   # If, For, While, Select
│   ├── arrays.test         # Array declaration, access, ForEach
│   ├── functions.test      # Function calls, returns
│   ├── classes.test        # Classes, properties, constructors
│   ├── generics.test       # Generic instantiation
│   ├── interfaces.test     # Interface implementation
│   ├── exceptions.test     # Try/Catch/Finally
│   └── string_funcs.test   # String runtime functions
└── integration/            # End-to-end tests
    ├── hello_world.test    # Full compile + run
    ├── hello_world_exe.test# Verify executable output
    └── complex.test        # Multi-file, multi-feature
```

## Running Tests

```bash
# From compiler/build directory
lit -v tests/

# Run specific test suite
lit -v tests/lexer/
lit -v tests/codegen/hello.test

# With custom test executable
lit -v --param bfc_exe=./bfc.exe tests/
```

## Test Format

Tests use the LLVM lit format (`.test` files):

```lit
; RUN: %bfc %s -o %t.exe
; RUN: %t.exe | FileCheck %s

Module Test
Sub Main()
    Print("Hello World")
End Sub
End Module

; CHECK: Hello World
```

## Test Categories

### Unit Tests (lexer, parser, semantic)
- Fast, no executable generation
- Verify AST, tokens, diagnostics

### Codegen Tests (codegen/)
- Generate .ll, compile to .exe, run
- Verify output with FileCheck

### Integration Tests (integration/)
- Full pipeline: source → .exe → execute
- Multi-file, complex scenarios

## Test Utilities

### lit.cfg.py
```python
import lit.formats
import os

config.name = "BasicForge"
config.test_format = lit.formats.ShTest()
config.suffixes = ['.test']
config.test_source_root = os.path.dirname(__file__)
config.excludes = ['lit.cfg.py', 'README.md']

# Substitutions
config.substitutions.append(('%bfc', os.path.join(config.test_exec_root, '..', 'bfc.exe')))
config.substitutions.append(('%t', 'test_output'))
```

---

## Writing New Tests

1. Create `.test` file in appropriate subdirectory
2. Use `; RUN:` for commands
2. Use `; CHECK:` for output verification
3. Use `%s` for current test file, `%t` for temp file

Example:
```lit
; Test: Integer addition
; RUN: %bfc %s -o %t.exe
; RUN: %t.exe | FileCheck %s

Module Test
Sub Main()
    Dim a As Integer = 5
    Dim b As Integer = 10
    Print(a + b)
End Sub
End Module

; CHECK: 15
```

---

## Continuous Integration

Tests run on every PR via GitHub Actions:

```yaml
- name: Run Compiler Tests
  run: |
    cd compiler/build
    lit -v --param bfc_exe=./bfc.exe ../tests/
```

---

## Test Coverage Goals

| Component | Target Coverage |
|-----------|-----------------|
| Lexer     | 100% keywords, 90% edge cases |
| Parser    | 95% grammar rules |
| Semantic  | 90% type rules |
| Codegen   | 90% IR opcodes |
| Runtime   | 100% exported functions |
| Integration| 100% language features |