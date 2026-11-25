# BasicForge Examples

All examples are in `examples/` and are valid BasicForge programs.

| Example | Description |
|---|---|
| `hello.bf` | Hello World |
| `HelloWorld.bf` | Hello World (alternate) |
| `math.bf` | Arithmetic and functions |
| `strings.bf` | String functions |
| `arrays.bf` | Arrays and loops |
| `loops.bf` | For/While loops |
| `conditions.bf` | If/Select Case |
| `classes.bf` | Classes, fields, methods, properties |
| `types.bf` | Types and conversions |
| `functions.bf` | Functions |
| `forms.bf` | Native Win32 form |

## Running an example

```powershell
bfc.exe examples\classes.bf -o classes.exe
.\classes.exe
```

## Standard library demo

```vb
Module Main
    Include "math.bf"
    Include "string.bf"

    Sub Main()
        Print(Clamp(15, 0, 10))     ' 10
        Print(IsPrime(97))          ' True
        Print(ToUpper("hello"))     ' HELLO
        Print(Reverse("abc"))       ' cba
    End Sub
End Module
```

```powershell
bfc.exe -I libraries/stdlib demo.bf -o demo.exe
```

## Form demo

```vb
Module Main
    Form MainForm
        Width = 640
        Height = 480
        Text = "Demo"

        Button btnOK
            Text = "OK"
            Left = 20
            Top = 20
            Width = 100
            Height = 32
            OnClick = btnOK_Click
        End Button
    End Form

    Sub Main()
    End Sub

    Sub btnOK_Click()
        Print("clicked")
    End Sub
End Module
```

## More examples

See `docs/USER_GUIDE.md` and the `tests/` directory for end-to-end tests.