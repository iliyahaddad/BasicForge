# BasicForge User Guide

BasicForge is a BASIC-like language with a native Windows x64 compiler, an
egui-based IDE, a Win32 form designer, an LSP server and a native debugger.

## Requirements

- Windows 10/11 x64
- LLVM/clang/lld-link (`clang.exe`, `lld-link.exe`) — auto-detected
- Visual Studio 2022 build tools (MSVC toolset) or clang-cl
- Rust toolchain (only to build the IDE)

## First program

Create `hello.bf`:

```vb
Module Main
    Sub Main()
        Print("Hello, BasicForge!")
    End Sub
End Module
```

Compile and run:

```powershell
bfc.exe hello.bf -o hello.exe
.\hello.exe
```

## CLI

```
bfc <command> [args...]
bfc <file.bf> [-o <output.exe>] [-I <include-dir>]

Commands:
  new <name>        Create a project
  build <project>   Build a project
  run <project>     Build and run
  check <file>      Syntax/semantic check
  clean <project>   Remove build artifacts
  fmt <file>        Format a source file
  test              Run tests in tests/
  doc <file>        Generate HTML documentation
  lsp | --lsp       Language server on stdio
  debug <exe>       Native debugger
```

### Include paths

`Include "math.bf"` is resolved relative to the including file, then every
`-I` directory, then `libraries/` and `libraries/stdlib/` next to `bfc.exe`.

```powershell
bfc.exe -I libraries/stdlib myapp.bf -o myapp.exe
```

## IDE

```powershell
cd ide
cargo run --release
```

The IDE provides:

- Syntax highlighting, completion and diagnostics
- Project explorer, build/run, output window
- Form Designer (drag/drop, property grid, events)
- Debugger launcher (starts `bfc debug` in a console)
- LSP client hooks

## Examples

See `examples/` and `docs/EXAMPLES.md`.

## Next steps

- `docs/LANGUAGE_REFERENCE.md`
- `docs/STDLIB.md`
- `docs/DEBUGGER_GUIDE.md`