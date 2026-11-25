# BasicForge Operators

**Version:** 0.1

BasicForge operators are English-like and follow Visual Basic conventions.
Operator matching is case-insensitive.

## Arithmetic operators

| Operator | Meaning | Operand types | Result type | Status |
|---|---|---|---|---|
| `+` | Addition | numeric | numeric | Codegen'd (Integer) |
| `-` | Subtraction | numeric | numeric | Parsed |
| `*` | Multiplication | numeric | numeric | Parsed |
| `/` | Division (floating point) | numeric | Double | Parsed |
| `\` | Integer division | Integer | Integer | Parsed |
| `Mod` | Modulo (remainder) | Integer | Integer | Parsed |
| `^` | Exponentiation (power) | numeric | Double | Parsed |
| `-` | Unary negation | numeric | numeric | Parsed |

Note: `/` always produces a `Double` (VB semantics). Use `\` for integer
division that truncates toward zero.

## String operator

| Operator | Meaning | Operand types | Result type | Status |
|---|---|---|---|---|
| `&` | Concatenation | String, String | String | Parsed |

Non-string operands are implicitly converted to String (planned).

## Comparison operators

| Operator | Meaning | Operand types | Result type | Status |
|---|---|---|---|---|
| `=` | Equal | same type | Boolean | Parsed |
| `<>` | Not equal | same type | Boolean | Parsed |
| `<` | Less than | numeric / String | Boolean | Parsed |
| `>` | Greater than | numeric / String | Boolean | Parsed |
| `<=` | Less than or equal | numeric / String | Boolean | Parsed |
| `>=` | Greater than or equal | numeric / String | Boolean | Parsed |

## Logical / bitwise operators

| Operator | Meaning | Operand types | Result type | Status |
|---|---|---|---|---|
| `Not` | Logical NOT (unary) | Boolean | Boolean | Tokenized |
| `And` | Logical AND | Boolean | Boolean | Tokenized |
| `Or` | Logical OR | Boolean | Boolean | Tokenized |
| `Xor` | Logical XOR | Boolean | Boolean | Tokenized |

Short-circuiting operators `AndAlso` and `OrElse` are planned (Milestone 3+).

## Operator precedence (highest to lowest)

1. `( )` — parentheses
2. `-` (unary negate), `Not` — unary
3. `^` — exponentiation (right-associative, planned)
4. `*`, `/`, `\`, `Mod` — multiplicative (left-associative)
5. `+`, `-`, `&` — additive / concatenation (left-associative)
6. `=`, `<>`, `<`, `>`, `<=`, `>=` — comparison (left-associative)
7. `And`, `Or`, `Xor` — logical (left-associative)

Use parentheses to override precedence explicitly.

## Examples

```vb
Dim a As Integer = 10
Dim b As Integer = 3

Print(a + b)      ' 13
Print(a - b)      ' 7
Print(a * b)      ' 30
Print(a / b)      ' 3.333333333333333 (Double, planned output)
Print(a \ b)      ' 3
Print(a Mod b)    ' 1
Print(a ^ 2)      ' 100 (planned)
Print("a" & "b")  ' "ab" (planned output)
Print(a = b)      ' False
Print(a <> b)     ' True
Print(a > b)      ' True
Print(Not True)   ' False
```
