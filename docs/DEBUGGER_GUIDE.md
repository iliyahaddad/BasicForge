# BasicForge Debugger Guide

`bfc debug <exe>` starts a native Win32 debugger. It uses the PDB produced by
`lld-link /debug` and `dbghelp.dll` for symbols and line information.

## Starting

```powershell
bfc.exe debug MyApp.exe
```

The debugger stops at process creation. Type `help` for commands.

## Commands

| Command | Alias | Description |
|---|---|---|
| `break <function>` | `b` | Break at a function |
| `break <file>:<line>` | `b` | Break at a source line |
| `break <name> if <cond>` | `b` | Conditional breakpoint |
| `continue` | `c` | Resume |
| `step` | `s` | Single-step one instruction |
| `registers` | `r` | Show registers |
| `setreg <name> <value>` | — | Edit a register (`rax`, `rip`, ...) |
| `disasm` | — | Disassemble the image (llvm-objdump) |
| `backtrace` | `bt` | Stack trace |
| `threads` | — | List threads |
| `delete <n>` | `d` | Remove a breakpoint |
| `disable <n>` / `enable <n>` | — | Toggle a breakpoint |
| `print <symbol\|0xADDR>` | `p` | Read a global/address |
| `locals` | `stack` | Current function + stack words |
| `help` | `h` | Help |
| `quit` | `q` | Terminate the debuggee |

## Example session

```
bfc.exe debug demo.exe
(bfdbg) break Main
Breakpoint 1 at Main (0x7ff6...)
(bfdbg) continue
Breakpoint 1: Main
(bfdbg) registers
Rip=0x7ff6... Rsp=0x...
(bfdbg) backtrace
  #0 0x7ff6... Main
(bfdbg) print MyGlobal
MyGlobal @ 0x... = 42
(bfdbg) quit
```

## Breakpoints

- Function breakpoints use the PDB symbol table (`SymFromName`).
- `file:line` breakpoints enumerate symbols and match `SymGetLineFromAddr64`.
- Up to 4 hardware breakpoints are supported (Dr0–Dr3).

## Conditional breakpoints

```
(bfdbg) break Main if rip != 0
(bfdbg) break MyFunc if rcx == 5
(bfdbg) break MyFunc if [0x1234] == 1
```

The condition supports `==`, `!=`, `>`, `<`, `>=`, `<=`, register names
(`rax`, `rcx`, `rip`, ...) and `[address]` memory reads.

## Locals and stack

`locals` prints the current function (from PDB symbols) and the 8 qwords at
`RSP`. Source-level local variable names from PDB local info are not decoded
yet.

## Limitations

- Global variables only (`print <symbol>`); locals require PDB local info.
- No conditional breakpoints yet.
- The IDE debugger panel drives this CLI and shows disassembly plus editable
  registers; source-level locals and a full call-stack variable view are not
  implemented yet.

## Building with debug info

`bfc` compiles the generated C with `-g` and links with `/debug`, so a PDB is
produced next to the executable. Keep the PDB with the EXE for symbol/line
information.