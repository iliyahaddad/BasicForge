# BasicForge IDE Guide

The IDE is a Rust/egui application in `ide/`.

## Build and run

```powershell
cd ide
cargo run --release
```

## Editor

- Syntax highlighting for BasicForge
- Line numbers, current-line highlight
- Diagnostics from the built-in analyzer
- Completion (Ctrl+Space)
- Go-to-definition and find references (built-in analyzer)
- Comment/uncomment (Ctrl+/), find/replace, go-to-line (Ctrl+G)

## Project explorer

- Create/open a folder
- Add new `.bf`, `.families`, `.prompts` files
- Build/run from the explorer
- Output window with diagnostics and build results

## Form Designer

- Drag controls from the toolbox onto the form
- Move/resize with the mouse
- Property grid: Text, Left, Top, Width, Height
- Events: Click, DoubleClick, TextChanged, SelectedIndexChanged
- Save/load `.bfform`
- **Generate Code** emits a compilable `Form ... End Form` program

## Debugger

The debugger panel can:

- Set breakpoints (UI data model)
- Launch the native debugger with **Start Debugger**
- The native `bfc debug` console opens in a new window

The native debugger supports breakpoints, step, continue, backtrace,
registers and globals (see `docs/DEBUGGER_GUIDE.md`).

## Settings

Edit `ide/src/config` or the in-app settings (theme, tab size, font, auto-save,
auto-build, line numbers, word wrap, LSP server path).

## Build from source

```powershell
cargo build --release
```

The binary is `ide/target/release/basicforge-ide.exe`.