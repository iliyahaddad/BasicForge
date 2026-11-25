# BasicForge Examples

Directory: `examples/`

## Milestone 2 (verified working)

| File | Status | Output |
|---|---|---|
| `HelloWorld.bf` | COMPILES + RUNS | `Hello World` |

## Milestone 3 examples (target)

| # | File | Compile status | Notes |
|---|---|---|---|
| 01 | `milestone3/01_hello_world.bf` | COMPILES | Verified PE32+ EXE |
| 02 | `milestone3/02_variables.bf` | PARSES + SEMANTIC | Print(identifier) not codegen'd |
| 03 | `milestone3/03_type_inference.bf` | PARSES + SEMANTIC | Print(identifier) not codegen'd |
| 04 | `milestone3/04_arithmetic.bf` | PARSES | Only `+` codegen'd |
| 05 | `milestone3/05_conditions.bf` | PARSES + SEMANTIC | If/Else not codegen'd |
| 06 | `milestone3/06_loops.bf` | PARSES | For body not codegen'd |
| 07 | `milestone3/07_functions.bf` | PARSES | Return/calls not codegen'd |
| 08 | `milestone3/08_modules.bf` | COMPILES | Single module |
| 09 | `milestone3/09_classes.bf` | PARSES (name only) | Class body not implemented |
| 10 | `milestone3/10_arrays.bf` | MISSING | No array support |
| 11 | `milestone3/11_strings.bf` | MISSING | No standard library |
| 12 | `milestone3/12_error_handling.bf` | MISSING | No error handling |

Each `.bf` file has a matching `.status` file explaining its state.

**How to compile:**
```
bfc <file>.bf -o <file>.exe
<file>.exe
```

## Conventions

- Examples that do not yet compile must NOT be claimed as working.
- `.status` files are the source of truth for compile status.
