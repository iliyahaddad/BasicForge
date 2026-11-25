@echo off
REM Build script for BasicForge IDE release
REM Creates both installer and portable package

set PROJECT_DIR=%~dp0
set BUILD_DIR=%PROJECT_DIR%\build
set RELEASE_DIR=%PROJECT_DIR%\release
set VERSION=0.1.0

echo ========================================
echo BasicForge IDE Release Build
echo Version: %VERSION%
echo ========================================

REM Clean previous builds
if exist "%RELEASE_DIR%" rmdir /s /q "%RELEASE_DIR%"
mkdir "%RELEASE_DIR%"

REM Build compiler (C++)
echo.
echo Building compiler...
cd /d "%PROJECT_DIR%\compiler"
if not exist "%BUILD_DIR%\compiler" mkdir "%BUILD_DIR%\compiler"
cd /d "%BUILD_DIR%\compiler"
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release -DBFC_BUILD_TESTS=ON "%PROJECT_DIR%\compiler"
if errorlevel 1 (
    echo Compiler CMake configuration failed!
    exit /b 1
)
cmake --build . --config Release --parallel
if errorlevel 1 (
    echo Compiler build failed!
    exit /b 1
)

REM Run compiler tests
echo.
echo Running compiler tests...
ctest -C Release --output-on-failure
if errorlevel 1 (
    echo Compiler tests failed!
    exit /b 1
)

REM Build IDE (Rust)
echo.
echo Building IDE...
cd /d "%PROJECT_DIR%\ide"
cargo build --release
if errorlevel 1 (
    echo IDE build failed!
    exit /b 1
)

REM Run IDE tests
echo.
echo Running IDE tests...
cargo test --release
if errorlevel 1 (
    echo IDE tests failed!
    exit /b 1
)

REM Run IDE benchmarks
echo.
echo Running IDE benchmarks...
cargo bench --release -- --save-baseline main
if errorlevel 1 (
    echo IDE benchmarks failed!
    exit /b 1
)

REM Copy artifacts to release directory
echo.
echo Packaging release...

REM Portable package
set PORTABLE_DIR=%RELEASE_DIR%\BasicForgeIDE_Portable_%VERSION%
mkdir "%PORTABLE_DIR%"
mkdir "%PORTABLE_DIR%\bin"
mkdir "%PORTABLE_DIR%\runtime"
mkdir "%PORTABLE_DIR%\sdks"
mkdir "%PORTABLE_DIR%\samples"

copy "%BUILD_DIR%\compiler\Release\bfc.exe" "%PORTABLE_DIR%\bin\"
copy "%PROJECT_DIR%\ide\target\release\basicforge-ide.exe" "%PORTABLE_DIR%\bin\"
xcopy "%PROJECT_DIR%\runtime" "%PORTABLE_DIR%\runtime\" /E /I /Y
xcopy "%PROJECT_DIR%\sdks" "%PORTABLE_DIR%\sdks\" /E /I /Y
xcopy "%PROJECT_DIR%\samples" "%PORTABLE_DIR%\samples\" /E /I /Y
copy "%PROJECT_DIR%\LICENSE" "%PORTABLE_DIR%\"
copy "%PROJECT_DIR%\README.md" "%PORTABLE_DIR%\"

REM Create portable launcher batch file
echo @echo off > "%PORTABLE_DIR%\BasicForgeIDE.bat"
echo set PATH=%%~dp0bin;%%PATH%% >> "%PORTABLE_DIR%\BasicForgeIDE.bat"
echo start "" "%%~dp0bin\basicforge-ide.exe" %%* >> "%PORTABLE_DIR%\BasicForgeIDE.bat"

REM Create ZIP for portable
cd /d "%RELEASE_DIR%"
powershell -Command "Compress-Archive -Path 'BasicForgeIDE_Portable_%VERSION%\*' -DestinationPath 'BasicForgeIDE_Portable_%VERSION%.zip' -Force"

REM Create installer using Inno Setup (if available)
where iscc >nul 2>&1
if %errorlevel% equ 0 (
    echo.
    echo Building installer with Inno Setup...
    iscc "%PROJECT_DIR%\installer.iss"
) else (
    echo.
    echo Inno Setup not found, skipping installer build.
    echo Install Inno Setup from https://jrsoftware.org/isinfo.php to create installer.
)

echo.
echo ========================================
echo Release build complete!
echo Portable package: %RELEASE_DIR%\BasicForgeIDE_Portable_%VERSION%.zip
if exist "%RELEASE_DIR%\BasicForgeIDE_Setup_%VERSION%.exe" (
    echo Installer: %RELEASE_DIR%\BasicForgeIDE_Setup_%VERSION%.exe
)
echo ========================================