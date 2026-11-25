# BasicForge SDKs

## LLVM / LLD

BasicForge uses LLVM/clang and lld-link as its backend toolchain.

### Quick setup (online)

```powershell
# Installs LLVM 19+ x64 (includes clang.exe and lld-link.exe)
winget install LLVM.LLVM
```

### Offline setup

1. Download the LLVM x64 installer on a connected machine.
2. Copy it to `sdks/llvm/`.
3. Run the installer with `/S` (silent) or extract it.
4. Set `BFC_LLVM_ROOT` (CMake) / `BFC_LLVM_BIN` (runtime).

### Fetch script

```powershell
.\scripts\fetch_sdk.ps1
```

The script checks for `clang.exe` and installs LLVM via winget if missing.

## Direct LLVM backend

The optional direct LLVM C++ backend requires the development libraries:

- `include/llvm/IR/Module.h`
- `lib/LLVM.lib`
- `lib/LLD.lib`

CMake auto-detects them and defines `BFC_LLVM_HEADERS_AVAILABLE`.

## Notes

- `bfc.exe` hides all LLVM/LLD details from the user.
- Generated applications do not require LLVM/LLD at runtime.