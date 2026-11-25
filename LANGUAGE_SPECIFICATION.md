# BasicForge Language Specification

**Version:** 0.1 (Milestone 3 draft)  
**Codename:** BasicForge  
**Target platform:** Windows 10/11 x64 (PE32+)  
**Syntax inspiration:** Visual Basic 6 (primary), Visual Basic .NET (selected improvements)

---

## 1. Introduction

BasicForge is a new, independent programming language. It is not VB6, not VB.NET,
and not a wrapper around any existing language. It combines:

- The readability and English-like block structure of Visual Basic 6.
- A strong static type system with optional type inference (VB.NET-style).
- Native x64 machine-code output via its own compiler frontend and IR.

BasicForge source files use the `.bf` extension. A program is compiled by
`bfc.exe` into a native Windows PE32+ executable. The generated application does
not require a separately installed BasicForge runtime, .NET runtime, or a C/C++
compiler on the end-user machine.

---

## 2. Lexical Structure

### 2.1 Character set
BasicForge source is UTF-8. The lexer recognizes ASCII letters (`A-Z`, `a-z`),
digits (`0-9`), underscore (`_`), whitespace (space, tab, carriage return,
newline), and printable punctuation.

### 2.2 Case sensitivity
**Keywords and identifiers are case-insensitive.** `MODULE`, `Module`, and
`module` are equivalent. `Print`, `PRINT`, and `print` are the same builtin.
Identifier *spelling* is preserved for display but compared case-insensitively.

### 2.3 Comments
A comment begins with a single quote `'` and runs to the end of the line:

```vb
' This is a comment
Dim x As Integer   ' inline comment
```

### 2.4 Identifiers
An identifier starts with a letter or underscore, followed by letters, digits,
or underscores. Identifiers may not coincide with reserved keywords (see
§4 Keywords).

### 2.5 Literals

| Kind | Syntax | Example |
|---|---|---|
| Integer | digits | `42`, `1000` |
| Long | digits with suffix `L` or `&` (future) | `42&` (planned) |
| Double | digits `.` digits | `3.14`, `125.50` |
| Boolean | `True` / `False` | `True` |
| String | double-quoted text; escapes `\"`, `\\`, `\0A` (LF), `\0D` (CR), `\09` (TAB) | `"Hello World"` |

### 2.6 Line structure
Statements are terminated by newline. A colon `:` may separate multiple
statements on one line. A trailing underscore `_` continues a statement on the
next line (line continuation, planned).

---

## 3. Program Structure

### 3.1 Module
A source file is a module:

```vb
Module Name

    Sub Main()
        Print("Hello")
    End Sub

End Module
```

- Every executable program must contain exactly one `Sub Main()` entry point
  in some module.
- Module names are identifiers.
- `End Module` closes the module (two tokens: `End` + `Module`).

### 3.2 Procedures
A module contains `Sub` (no return value) and `Function` (returns a value)
procedures:

```vb
Sub SayHello(name As String)
    Print("Hello " & name)
End Sub

Function Add(a As Integer, b As Integer) As Integer
    Return a + b
End Function
```

- `End Sub` / `End Function` close a procedure (two tokens each).
- Parameters are listed in parentheses, separated by commas.
- Each parameter has a name and an explicit type (`As type`).

---

## 4. Keywords

The following are reserved and may not be used as identifiers:

```
And, As, Boolean, Case, Class, Dim, Double, Else, End, False, For,
Function, If, Integer, Long, Module, New, Next, Not, Nothing, Or, Print,
Private, Property, Public, Return, Select, Single, Step, String, Sub, Then,
True, While, Xor
```

Block terminators are written as two tokens: `End Sub`, `End Function`,
`End Class`, `End If`, `End While`, `End Select`, `End Module`. `Case Else`
is written as two tokens.

---

## 5. Types

### 5.1 Primitive types

| Type | Size | Range / values | Literal suffix |
|---|---|---|---|
| Integer | 32-bit signed | -2,147,483,648 .. 2,147,483,647 | none |
| Long | 64-bit signed | -2^63 .. 2^63-1 | `&` (planned) |
| Single | 32-bit float | IEEE-754 | none |
| Double | 64-bit float | IEEE-754 | none |
| Boolean | 1 bit | `True`, `False` | none |
| String | UTF-8 sequence | any text | `"..."` |
| Void | — | no value (procedures only) | — |

### 5.2 Type inference
`Dim x = 42` infers `Integer`. `Dim s = "hi"` infers `String`. The initializer
must be present and its type determines the variable's type.

### 5.3 User-defined types
Classes, structures, enums, and interfaces are planned for Milestone 4+ and are
not part of this specification version.

---

## 6. Statements

### 6.1 Declaration
```vb
Dim name As type [= initializer]
Dim name = initializer        ' type inference
```

### 6.2 Assignment
```vb
name = expression
```
(Assignment statement planned for Milestone 3.)

### 6.3 Print
```vb
Print(expression [, expression ...])
```
Prints each argument to the console. `Print` is a builtin procedure.

### 6.4 Return
```vb
Return            ' in a Sub
Return expression ' in a Function
```

### 6.5 If / Then / Else / End If
```vb
If condition Then
    statements
Else
    statements
End If
```
The `Else` branch is optional. The condition must be of type `Boolean`.

### 6.6 Select Case
```vb
Select Case subject
    Case value1, value2
        statements
    Case Else
        statements
End Select
```
`Case Else` is the default branch.

### 6.7 For / To / Step / Next
```vb
For variable As type = start To end [Step step]
    statements
Next
```
`Step` defaults to 1. The loop variable is local to the loop.

### 6.8 While / End While
```vb
While condition
    statements
End While
```
The condition must be of type `Boolean`.

### 6.9 Expression statement
A procedure call used as a statement:
```vb
Print("x")
```

---

## 7. Expressions

### 7.1 Precedence (highest to lowest)
1. Parentheses `( ... )`
2. Unary `-` (negate), `Not`
3. `^` (power), `*` (multiply), `/` (divide), `\` (integer divide), `Mod` (modulo)
4. `+` (add), `-` (subtract), `&` (string concatenation)
5. `=`, `<>`, `<`, `>`, `<=`, `>=` (comparison)
6. `And`, `Or`, `Xor` (logical / bitwise)

All binary operators are left-associative except `^`, which is right-associative
(planned).

### 7.2 Boolean operators
`And`, `Or`, `Not`, `Xor` operate on Boolean values. Short-circuiting operators
`AndAlso` / `OrElse` are planned.

### 7.3 String concatenation
`&` concatenates two strings. Non-string operands are implicitly converted to
String (planned conversion rules).

---

## 8. Scope and Lifetime

- Module-level names: procedures, classes defined in a module.
- Procedure-level names: parameters and `Dim` variables.
- Variables are in scope from their declaration to the end of the enclosing
  block/procedure.
- Shadowing a parameter or outer variable is an error (planned).

---

## 9. Type Conversion (planned)

- Widening conversions (Integer → Long → Double, Integer → String) are implicit.
- Narrowing conversions require an explicit conversion function (planned:
  `CInt`, `CLng`, `CDbl`, `CStr`, `CBool`).
- No implicit conversion between Boolean and numeric types.

---

## 10. Builtin Procedures

| Procedure | Meaning |
|---|---|
| `Print(arg, ...)` | Write arguments to console |

Standard string functions (`Left`, `Right`, `Mid`, `InStr`, `Len`, `Trim`,
`Replace`, `Split`, `Join`, `Format`) and conversion functions (`CInt`, `CLng`,
`CDbl`, `CStr`, `CBool`) are planned for the standard library (Milestone 5+).

---

## 11. Program Entry Point

Every executable BasicForge program must define exactly one `Sub Main()` (no
parameters, no return value) in some module. Execution begins at `Main`.

---

## 12. Error Handling

Structured exception handling (`Try / Catch / Finally`) is planned for
Milestone 5+. Until then, runtime errors terminate the process with a non-zero
exit code.

---

## 13. Source File Encoding and Extensions

- Source files: `.bf`, UTF-8.
- Project files: `.bfproj` (human-readable, planned).
- Form files: `.bfform` (planned, visual designer).
- Resource files: `.bfres` (planned).

---

## 14. Conformance

A BasicForge implementation accepts exactly the syntax and semantics described
in this document. Programs that compile under this specification must produce a
valid PE32+ x64 executable whose observable behavior matches the semantics
defined here (print output, arithmetic results, control flow).
