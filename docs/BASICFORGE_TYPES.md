# BasicForge Types

**Version:** 0.1

BasicForge is strongly and statically typed. Every variable, parameter, and
function return value has a type known at compile time.

## Primitive types

| Type | Keyword | Size | Representation | Literal example |
|---|---|---|---|---|
| Integer | `Integer` | 32 bits | two's complement signed | `42`, `-7`, `0` |
| Long | `Long` | 64 bits | two's complement signed | `42&` (planned) |
| Single | `Single` | 32 bits | IEEE-754 binary32 | `3.14F` (planned) |
| Double | `Double` | 64 bits | IEEE-754 binary64 | `3.14`, `125.50` |
| Boolean | `Boolean` | 1 bit | `True` / `False` | `True`, `False` |
| String | `String` | variable | UTF-8 sequence | `"Hello"` |
| Void | `Void` | — | no value | — (procedures only) |

## Type declaration syntax

Explicit type:
```vb
Dim age As Integer
Dim name As String
Dim enabled As Boolean
Dim price As Double
```

Type inference (initializer required):
```vb
Dim total = 100          ' inferred Integer
Dim message = "Hello"    ' inferred String
Dim flag = True          ' inferred Boolean
Dim ratio = 1.5          ' inferred Double
```

When both `As type` and an initializer are present, the initializer's type must
match the declared type exactly (no implicit conversion yet).

## Type compatibility

- Assignment requires identical types (widening/narrowing conversions planned).
- `Print` accepts `String` and `Integer` in the current slice; all primitive
  types planned.
- `If` and `While` conditions must be `Boolean`.
- Comparison operators require both operands to have the same type (planned:
  numeric types may be compared after widening).
- Arithmetic operators require numeric operands (`Integer`, `Long`, `Single`,
  `Double`). Result type follows the widest operand (planned).
- `&` (concatenation) requires string operands (implicit conversion planned).

## Void

`Void` is used only as a `Sub` return type (meaning no return value). It may not
be used for variables or parameters.

## User-defined types

Classes, structures (`Structure`), and enumerations (`Enum`) are planned for
Milestone 4+. Arrays (`Dim arr(n) As type`) are planned for Milestone 4.

## Conversion functions (planned)

`CInt`, `CLng`, `CDbl`, `CStr`, `CBool` for explicit conversion. Implicit
widening: `Integer` → `Long` → `Single` → `Double` → `String`.
