#!/usr/bin/env pwsh
# Build script for BasicForge IDE release (PowerShell)
# Creates both installer and portable package

param(
    [string]$Version = "0.1.0",
    [switch]$SkipTests,
    [switch]$SkipBenchmarks
)

$ErrorActionPreference = "Stop"

$ProjectDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$BuildDir = Join-Path $ProjectDir "build"
$ReleaseDir = Join-Path $ProjectDir "release"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "BasicForge IDE Release Build" -ForegroundColor Cyan
Write-Host "Version: $Version" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

# Clean previous builds
if (Test-Path $ReleaseDir) {
    Remove-Item -Recurse -Force $ReleaseDir
}
New-Item -ItemType Directory -Path $ReleaseDir | Out-Null

# Build compiler (C++)
Write-Host "`nBuilding compiler..." -ForegroundColor Green
Set-Location (Join-Path $ProjectDir "compiler")
if (-not (Test-Path (Join-Path $BuildDir "compiler"))) {
    New-Item -ItemType Directory -Path (Join-Path $BuildDir "compiler") | Out-Null
}
Set-Location (Join-Path $BuildDir "compiler")
& cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release -DBFC_BUILD_TESTS=ON (Join-Path $ProjectDir "compiler")
if ($LASTEXITCODE -ne 0) { throw "Compiler CMake configuration failed!" }
& cmake --build . --config Release --parallel
if ($LASTEXITCODE -ne 0) { throw "Compiler build failed!" }

# Run compiler tests
if (-not $SkipTests) {
    Write-Host "`nRunning compiler tests..." -ForegroundColor Green
    & ctest -C Release --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw "Compiler tests failed!" }
}

# Build IDE (Rust)
Write-Host "`nBuilding IDE..." -ForegroundColor Green
Set-Location (Join-Path $ProjectDir "ide")
& cargo build --release
if ($LASTEXITCODE -ne 0) { throw "IDE build failed!" }

# Run IDE tests
if (-not $SkipTests) {
    Write-Host "`nRunning IDE tests..." -ForegroundColor Green
    & cargo test --release
    if ($LASTEXITCODE -ne 0) { throw "IDE tests failed!" }
}

# Run IDE benchmarks
if (-not $SkipBenchmarks) {
    Write-Host "`nRunning IDE benchmarks..." -ForegroundColor Green
    & cargo bench --release -- --save-baseline main
    if ($LASTEXITCODE -ne 0) { throw "IDE benchmarks failed!" }
}

# Copy artifacts to release directory
Write-Host "`nPackaging release..." -ForegroundColor Green

$PortableDir = Join-Path $ReleaseDir "BasicForgeIDE_Portable_$Version"
New-Item -ItemType Directory -Path $PortableDir | Out-Null
New-Item -ItemType Directory -Path (Join-Path $PortableDir "bin") | Out-Null
New-Item -ItemType Directory -Path (Join-Path $PortableDir "runtime") | Out-Null
New-Item -ItemType Directory -Path (Join-Path $PortableDir "sdks") | Out-Null
New-Item -ItemType Directory -Path (Join-Path $PortableDir "samples") | Out-Null

Copy-Item (Join-Path $BuildDir "compiler\Release\bfc.exe") (Join-Path $PortableDir "bin\")
Copy-Item (Join-Path $ProjectDir "ide\target\release\basicforge-ide.exe") (Join-Path $PortableDir "bin\")
Copy-Item (Join-Path $ProjectDir "runtime") (Join-Path $PortableDir "runtime") -Recurse -Force
Copy-Item (Join-Path $ProjectDir "sdks") (Join-Path $PortableDir "sdks") -Recurse -Force
Copy-Item (Join-Path $ProjectDir "samples") (Join-Path $PortableDir "samples") -Recurse -Force
Copy-Item (Join-Path $ProjectDir "LICENSE") $PortableDir
Copy-Item (Join-Path $ProjectDir "README.md") $PortableDir

# Create portable launcher batch file
$launcherContent = @"
@echo off
set PATH=%~dp0bin;%PATH%
start "" "%~dp0bin\basicforge-ide.exe" %*
"@
Set-Content -Path (Join-Path $PortableDir "BasicForgeIDE.bat") -Value $launcherContent -Encoding ASCII

# Create ZIP for portable
Set-Location $ReleaseDir
Compress-Archive -Path (Join-Path $ReleaseDir "BasicForgeIDE_Portable_$Version\*") -DestinationPath (Join-Path $ReleaseDir "BasicForgeIDE_Portable_$Version.zip") -Force

# Create installer using Inno Setup (if available)
$isccPath = (Get-Command iscc -ErrorAction SilentlyContinue).Source
if ($isccPath) {
    Write-Host "`nBuilding installer with Inno Setup..." -ForegroundColor Green
    & iscc (Join-Path $ProjectDir "installer.iss")
} else {
    Write-Host "`nInno Setup not found, skipping installer build." -ForegroundColor Yellow
    Write-Host "Install Inno Setup from https://jrsoftware.org/isinfo.php to create installer." -ForegroundColor Yellow
}

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "Release build complete!" -ForegroundColor Cyan
Write-Host "Portable package: $ReleaseDir\BasicForgeIDE_Portable_$Version.zip" -ForegroundColor Cyan
$installerPath = Join-Path $ReleaseDir "BasicForgeIDE_Setup_$Version.exe"
if (Test-Path $installerPath) {
    Write-Host "Installer: $installerPath" -ForegroundColor Cyan
}
Write-Host "========================================" -ForegroundColor Cyan