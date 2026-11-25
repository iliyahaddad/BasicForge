# BasicForge Keywords

**Version:** 0.1

Keywords are reserved and may not be used as identifiers. Keyword matching is
case-insensitive (`Module`, `MODULE`, and `module` are equivalent).

Block terminators are written as **two tokens**: `End` followed by the block
keyword (for example `End Sub`, `End If`, `End While`, `End Select`,
`End Module`, `End Function`, `End Class`). `Case Else` is likewise two tokens.

## Reserved keywords

| Keyword | Purpose | Status |
|---|---|---|
| And | Logical / bitwise AND | Tokenized; partially codegen'd |
| As | Type annotation in `Dim` and parameters | Implemented |
| Boolean | Boolean type | Implemented |
| Case | Case in `Select Case` | Parsed |
| Class | Class declaration | Name only parsed |
| Dim | Variable declaration | Implemented |
| Double | 64-bit float type | Implemented |
| Else | Else branch of `If` / `Select Case` | Implemented |
| End | Block terminator prefix | Implemented |
| False | Boolean false literal | Implemented |
| For | `For ... To ... Next` loop | Parsed |
| Function | Function procedure | Parsed |
| If | Conditional statement | Parsed |
| Integer | 32-bit integer type | Implemented |
| Long | 64-bit integer type | Implemented |
| Module | Module declaration | Implemented |
| New | Constructor / object creation (planned) | Token only |
| Next | `For` loop terminator | Parsed |
| Not | Logical NOT | Tokenized |
| Nothing | Null reference (planned) | Token only |
| Or | Logical / bitwise OR | Tokenized |
| Print | Builtin console output | Implemented |
| Private | Access modifier (planned) | Token only |
| Property | Property declaration (planned) | Token only |
| Public | Access modifier (planned) | Token only |
| Return | Return from procedure | Parsed |
| Select | `Select Case` statement | Parsed |
| Single | 32-bit float type | Implemented |
| Step | `For` loop increment | Parsed |
| String | String type | Implemented |
| Sub | Sub procedure | Implemented |
| Then | `If` condition keyword | Parsed |
| True | Boolean true literal | Implemented |
| While | `While ... End While` loop | Parsed |
| Xor | Logical / bitwise XOR | Tokenized |

## Planned keywords (Milestone 4+)

`Const`, `Each`, `In`, `Do`, `Loop`, `While` (Wend variant), `Try`, `Catch`,
`Finally`, `Throw`, `Enum`, `Structure`, `Interface`, `Inherits`, `Imports`,
`Namespace`, `Shared`, `Static`, `ByVal`, `ByRef`, `Optional`, `ParamArray`,
`CInt`, `CLng`, `CDbl`, `CStr`, `CBool`, `AndAlso`, `OrElse`, `REM`, `Exit`,
`Continue`.

## Notes

- `EndIf`, `EndSub`, `EndFunction`, `EndClass`, `EndWhile`, `EndSelect` are
  **not** single tokens. They are always written as two tokens (`End Sub`, etc.).
  The earlier single-token forms were removed for consistency with VB6/VB.NET.
