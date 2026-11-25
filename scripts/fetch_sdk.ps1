$ErrorActionPreference = "Continue"
$llvmRoot = Join-Path $PSScriptRoot "..\sdks\llvm"
$clang = Join-Path $llvmRoot "bin\clang.exe"

if (Test-Path $clang) {
    Write-Host "LLVM already present: $clang"
    exit 0
}

Write-Host "LLVM not found. Installing via winget..."
try {
    winget install --id LLVM.LLVM -e --accept-source-agreements --accept-package-agreements
} catch {
    Write-Host "winget failed: $_"
    Write-Host "Download LLVM manually from https://github.com/llvm/llvm-project/releases"
    exit 1
}

if (Test-Path "C:\Program Files\LLVM\bin\clang.exe") {
    Write-Host "LLVM installed at C:\Program Files\LLVM"
} else {
    Write-Host "LLVM installed but clang.exe not found in the default path."
}