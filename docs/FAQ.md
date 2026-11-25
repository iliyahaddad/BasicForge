# BasicForge FAQ

### Is BasicForge a BASIC interpreter?

No. It has its own lexer, parser, semantic analyzer, IR and native code
generator. It emits C and compiles it with clang/lld-link (or uses the optional
LLVM backend).

### Which platforms are supported?

Windows 10/11 x64 only. This is intentional.

### Do I need LLVM?

Yes for native executables. `clang.exe` and `lld-link.exe` are auto-detected
from `BFC_LLVM_ROOT`, `BFC_LLVM_BIN` or `PATH`.

### How do I use libraries?

```vb
Module Main
    Include "math.bf"
    Sub Main()
        Print(Clamp(1, 0, 10))
    End Sub
End Module
```

Compile with `-I libraries/stdlib`.

### Why is my program not printing anything?

`Print` adds a newline and flushes. If you use a form, `bf_form_run` blocks
until the last window closes; put `Print` before it.

### How do I debug?

```powershell
bfc.exe debug MyApp.exe
(bfdbg) break Main
(bfdbg) continue
```

See `docs/DEBUGGER_GUIDE.md`.

### Does the LSP work with VS Code?

Yes. Configure:

```json
{
  "bf": {
    "command": "C:\\...\\BasicForge\\bfc.exe",
    "args": ["--lsp"]
  }
}
```

### Why does my form not appear?

Forms require `user32.lib`, `gdi32.lib`, `shell32.lib` and `comctl32.lib`.
`bfc` links them automatically. Make sure the form is declared at module level
and the program has a `Sub Main`.

### Can I use `And`/`Or` in `If`?

Yes. `And`, `Or`, `Xor` and `Not` are supported with full precedence.

### Why is `IsNumber("12a")` false?

`IsNumber` checks each character with `Asc` (digits only). Use `IsNumber` from
`validation.bf`.

### How do I contribute?

See `CONTRIBUTING.md`. Build with CMake and run `ctest`.