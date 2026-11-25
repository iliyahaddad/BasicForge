# BasicForge Syntax Reference

**Version:** 0.1  
**Primary reference:** Visual Basic 6  
**Selected improvements from:** Visual Basic .NET

This document shows the concrete syntax for each language construct, with the
current implementation status (see docs/LANGUAGE_COMPATIBILITY_AUDIT.md).

**Status key:**
- `IMPLEMENTED` — lexer, parser, semantic analysis, and codegen all work; a compiling example exists.
- `PARSES` — lexer and parser accept it; semantic analysis may check it; codegen does not emit it.
- `MISSING` — not implemented.

---

## 1. Module

```vb
Module Name

    Sub Main()
        Print("Hello")
    End Sub

End Module
```

Status: IMPLEMENTED (single module with `Sub Main`).

## 2. Sub procedure

```vb
Sub Name(param1 As type1, param2 As type2)
    statements
End Sub
```

Status: IMPLEMENTED (no return value).

## 3. Function procedure

```vb
Function Name(param1 As type1, param2 As type2) As returnType
    statements
    Return expression
End Function
```

Status: PARSES (parsed and type-checked; return value and `Return` not yet codegen'd).

## 4. Variable declaration (Dim)

```vb
Dim name As type
Dim name As type = initializer
Dim name = initializer        ' type inference (VB.NET-style)
```

Status: IMPLEMENTED (declaration + type inference).

## 5. Assignment

```vb
name = expression
```

Status: MISSING (parsed as expression statement only; no assignment storage).

## 6. Print

```vb
Print(expression1, expression2, ...)
```

Status: IMPLEMENTED for string literals; PARTIAL for other expressions.

## 7. Return

```vb
Return                    ' Sub
Return expression         ' Function
```

Status: PARSES (parsed; not codegen'd).

## 8. If / Then / Else / End If

```vb
If condition Then
    statements
Else
    statements
End If
```

Status: PARSES (condition type-checked as Boolean; blocks not codegen'd).

## 9. Select Case

```vb
Select Case subject
    Case value1, value2
        statements
    Case Else
        statements
End Select
```

Status: PARSES (parsed; not codegen'd).

## 10. For / To / Step / Next

```vb
For variable As type = start To end [Step step]
    statements
Next
```

Status: PARSES (parsed; not codegen'd). `Step` defaults to 1.

## 11. While / End While

```vb
While condition
    statements
End While
```

Status: PARSES (condition type-checked as Boolean; body not codegen'd).

## 12. Expression statement (call as statement)

```vb
Print("Hello")
```

Status: IMPLEMENTED for Print with literal argument.

## 13. Comments

```vb
' line comment
```

Status: IMPLEMENTED (`'` only; `REM` planned).

## 14. Literals

```vb
42               ' Integer
3.14             ' Double
True             ' Boolean
"Hello"          ' String
```

Status: IMPLEMENTED.

## 15. Binary operators

```vb
a + b    a - b    a * b    a / b    a \ b    a Mod b    a ^ b
a & b    a = b    a <> b   a < b    a > b    a <= b     a >= b
a And b  a Or b   a Xor b
```

Status: PARSED; only `+` codegen'd in current slice.

## 16. Unary operators

```vb
-a
Not b
```

Status: PARSED; only `-` partially codegen'd.

## 17. Planned (not in this spec version)

- `Const` declarations
- `For Each ... In ... Next`
- `Do ... Loop` / `Do While ... Loop`
- `Class ... End Class`, `Property`, `Sub New`
- `Enum ... End Enum`, `Structure ... End Structure`
- `Public` / `Private` / `Protected` / `Friend` access modifiers
- Arrays `Dim arr(n) As type`
- `Try / Catch / Finally`
- Standard library functions (`Len`, `Trim`, `Left`, `Right`, `Mid`, `InStr`,
  `Replace`, `Split`, `Join`, `Format`)
