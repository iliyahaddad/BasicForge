# BasicForge Language Reference

## Program structure

```vb
Module Main
    ' declarations
    Sub Main()
    End Sub
End Module
```

Modules, namespaces, classes, structures, enums, interfaces, delegates and
forms are supported.

## Types

| Type | Notes |
|---|---|
| `Integer` | 64-bit signed integer |
| `Long` | 64-bit signed integer |
| `Single` | 32-bit float |
| `Double` | 64-bit float |
| `Boolean` | `True` / `False` |
| `String` | Immutable text |

Variables:

```vb
Dim x As Integer
Dim y As Integer = 10
Dim name As String = "BasicForge"
Dim values(10) As Integer
```

## Statements

- `Dim`, `Const`
- Assignment `=`
- `If ... Then ... [Else ...] [End If]` (single-line supported)
- `Select Case ... Case ... [Case a To b] ... [Case Else] ... End Select`
- `For i = a To b [Step n] ... Next`
- `For Each item In array ... Next`
- `While ... End While`
- `Do ... Loop [While|Until ...]`
- `Try ... Catch ... Finally ... End Try`
- `Return`, `Exit`, `Throw`

## String interpolation

```vb
Dim name As String = "BasicForge"
Print($"Hello {name}!")
```

`$"..."` embeds expressions in `{...}`; `{{` and `}}` escape literal braces.

## Operators

| Operator | Meaning |
|---|---|
| `+ - * /` | arithmetic (`/` yields Double) |
| `\` | integer division |
| `Mod` | remainder |
| `^` | power |
| `&` | string concatenation (numbers auto-convert) |
| `= <>` | equality / inequality |
| `< <= > >=` | comparison |
| `And Or Xor Not` | logical |
| `Is` | identity (reference) |

## Procedures

```vb
Sub Greet(name As String)
    Print("Hello " & name)
End Sub

Function Add(a As Integer, b As Integer) As Integer
    Return a + b
End Function
```

Parameters support `ByVal`, `ByRef`, `Optional` and array parameters:

```vb
Sub SortAsc(arr() As Integer, n As Integer)
    ' ...
End Sub
```

## Classes

```vb
Module Main
    Class Counter
        Public Value As Integer

        Public Sub New(start As Integer)
            Value = start
        End Sub

        Public Sub Add(ByVal n As Integer)
            Value = Value + n
        End Sub

        Public Function Get() As Integer
            Return Value
        End Function
    End Class

    Sub Main()
        Dim c As New Counter(10)
        c.Add(5)
        Print(c.Get())
    End Sub
End Module
```

Fields, properties (`Public Property`), methods, constructors and `Me` are
supported.

## Arrays

```vb
Dim a(5) As Integer
a(0) = 10
Print(a(0))
Dim sum As Integer = 0
For i = 0 To 4
    sum = sum + a(i)
Next
```

## Forms

```vb
Form MainForm
    Width = 640
    Height = 480
    Text = "My App"

    Button btnOK
        Text = "OK"
        Left = 20
        Top = 20
        Width = 100
        Height = 32
        OnClick = btnOK_Click
    End Button
End Form

Sub btnOK_Click()
    Print("clicked")
End Sub
```

Controls: Button, Label, TextBox, CheckBox, RadioButton, GroupBox, Panel,
PictureBox, ListBox, ComboBox, TrackBar, ProgressBar, DateTimePicker,
ListView, TreeView.

Events: `OnClick`, `OnDoubleClick`, `OnTextChanged`, `OnSelectedIndexChanged`,
`OnValueChanged`, `OnAfterSelect` and `OnScroll`.

## Comments

```vb
' single-line comment
REM also a comment
```

## Includes

```vb
Module Main
    Include "math.bf"
    Include "string.bf"
    Sub Main()
        Print(Clamp(15, 0, 10))
    End Sub
End Module
```