# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- CI workflows: `release.yml`, `docs.yml`, `lint.yml`, `nightly.yml`.
- Documentation set: user guide, language reference, stdlib, IDE, debugger,
  examples, FAQ, release checklist.
- Standard library: `bitwise.bf`, `math_ex.bf`, `strings_ex.bf`.
- Debugger: full expression evaluator for conditional breakpoints
  (`&&`, `||`, `+`, `-`, `*`, `/`, `%`, comparisons, parentheses, `[addr]`),
  `locals`/`stack` (function + stack words + best-effort PDB local names),
  `setreg`, `disasm`, `threads`, `delete`, `enable`/`disable`.
- LSP: cross-file classes/methods and member access types; external method
  return types; unary minus preserves operand type; cross-file type checking
  reports mismatches.
- Multi-target compilation: `--target console|gui|sharedlib|staticlib`.
- Standard library: APL-style array operations (`IotaArray`, `ReduceSum`,
  `ReduceProduct`, `ReduceMax`, `ReduceMin`, `ScanSum`, `FilterEven`,
  `SelectGreater`).
- Form Designer controls: RichTextBox, TabControl, ToolBar, StatusBar, UpDown
  (in addition to the previous 15).
- Debugger: `watch`/`unwatch` with full expression evaluation.
- Include system: `Include "file.bf"` (recursive, cycle-safe, deduplicated) and
  the `-I <dir>` compiler option.
- Standard library: `math`, `string`, `array`, `file`, `date`, `console`,
  `json`, `csv`, `validation`, `random`.
- SDKs: `sdks/win32` (Win32 wrappers), `environment`, `json`, `csv`.
- Form Designer controls: TrackBar, ProgressBar, DateTimePicker, ListView,
  TreeView. Events: DoubleClick, SelectedIndexChanged, ValueChanged,
  AfterSelect, Scroll, GotFocus, LostFocus, KeyDown, KeyUp, MouseMove,
  MouseEnter and MouseLeave.
- IDE debugger panel: editable registers (`setreg`), disassembly view
  (`disasm`), breakpoints, call stack and live output.
- LSP hover/definition: workspace-wide symbol index with function signatures,
  parameter types and class members; variable types are now recorded.
- Complete language frontend (lexer, parser, semantic analysis, IR)
- C emission backend with LLVM/clang toolchain integration
- Native Windows x64 PE32+ executable generation
- Rust IDE with syntax highlighting, completion, and diagnostics
- Tree-sitter incremental parsing
- Salsa incremental semantic analysis
- Form Designer UI and native Win32 forms: `Form` declarations with Button,
  Label, TextBox, CheckBox, RadioButton, GroupBox, Panel, PictureBox, ListBox,
  ComboBox; Click/TextChanged events; `GetText`/`SetText`.
- Native debugger CLI: `bfc debug <exe>` with INT3 breakpoints (function or
  `file:line`), continue, single-step, backtrace, registers and global reads.
- LSP server (`bfc --lsp`) with diagnostics, keyword completion, hover and
  go-to-definition.
- Command-line interface (new, build, run, check, clean, fmt, test, doc, debug)
- Test suites: lexer, parser, semantic, IR, diagnostics, fuzz and end-to-end
  (`ctest` + `compiler/tests/run_tests.ps1`)
- Fuzzing infrastructure
- Static analysis configuration
- Performance benchmarks

### Fixed
- Class fields, properties, methods, constructors and `As New` now lower and
  execute correctly end-to-end.
- Method calls (`obj.Method(args)`) parse as calls instead of field assignments.
- Bare field access/assignment inside class methods resolves to `Me.field`.
- `ByVal`/`ByRef` parameter modifiers parse correctly.
- VB-style `&` concatenation converts numeric operands to text.
- Incremental build cache now includes a codegen version, so stale `.obj`
  files are not reused after a compiler upgrade.
- Generated C is compiled with `-fno-builtin-malloc` to avoid a miscompile of
  int64 object references.
- LLVM/clang/lld-link paths are configurable (`BFC_LLVM_ROOT`, `BFC_LLVM_BIN`).
- Fixed test namespace/API usage and made CTest able to locate `bfc.exe`.
- Array parameters (`arr() As T`) are parsed and passed as pointers; indexed
  reads inside functions now lower to `ArrayLoad`.
- Added the `<>` not-equal operator, `Or`/`Not` keywords, single-line
  `If ... Then ...`, and `=` as equality inside expressions.
- `For` loops with a negative `Step` now use the correct loop condition.
- The entry point prefers `Sub Main` even when library/procedure declarations
  are included before it.
- String comparisons in the standard library use character codes (`Asc`) so
  `IsNumber` works correctly.
- Lexer position tracking: `skip_comment` rewound only `pos_`, which doubled
  line/column counts and produced wrong diagnostic/LSP locations.
- Array reads `a(i)` are now lowered as `ArrayLoad` instead of calls, and
  `Case a To b` ranges are supported.
- `Single`/`Double` values print correctly (`bf_print_double`, `CStrDouble`).
- Linker now includes `user32.lib`, `gdi32.lib`, `kernel32.lib` and
  `libvcruntime.lib` (after objects) for form programs.
- CMake uses SDK 10.0.26100.0 include/lib paths and `BFC_LLVM_ROOT`.

### Changed
- N/A

### Deprecated
- N/A

### Removed
- N/A

### Fixed
- Empty label bug in LLVM IR generation
- Parser issues with classes, For Each, Try/Catch
- Enum value resolution

### Security
- Input validation for malformed source code
- Safe file handling

## [0.1.0] - 2026-11-17

### Added
- Initial release
- Basic language features (variables, arithmetic, control flow, functions, arrays)
- Hello World example
- Basic IDE