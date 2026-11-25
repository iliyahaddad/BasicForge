# BasicForge Compiler Test Runner
# Usage: .\run_tests.ps1

$BFC = "C:\Users\manshadi.PDF\Desktop\BasicForge\compiler\build\Release\bfc.exe"
$TESTS_DIR = "C:\Users\manshadi.PDF\Desktop\BasicForge\compiler\tests"
$PASS = 0
$FAIL = 0

function Test-File($name, $expectedOutput) {
    $bfFile = Join-Path $TESTS_DIR "$name.bf"
    $exeFile = Join-Path $TESTS_DIR "$name.exe"

    Write-Host "Testing $name..." -ForegroundColor Cyan

    # Compile
    $compileResult = & $BFC $bfFile -o $exeFile 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Host "  FAIL: Compilation failed" -ForegroundColor Red
        $compileResult | ForEach-Object { Write-Host "    $_" }
        $script:FAIL++
        return
    }

    # Run
    if (Test-Path $exeFile) {
        $runResult = & $exeFile 2>&1
        $actual = ($runResult | Out-String).Trim()
        $expected = $expectedOutput.Trim()

        if ($actual -eq $expected) {
            Write-Host "  PASS" -ForegroundColor Green
            $script:PASS++
        } else {
            # Normalize line endings for comparison
            $actualNorm = $actual -replace "`r`n", "`n"
            $expectedNorm = $expected -replace "`r`n", "`n"
            if ($actualNorm -eq $expectedNorm) {
                Write-Host "  PASS" -ForegroundColor Green
                $script:PASS++
            } else {
                Write-Host "  FAIL: Output mismatch" -ForegroundColor Red
                Write-Host "    Expected: '$expectedNorm'" -ForegroundColor Yellow
                Write-Host "    Actual:   '$actualNorm'" -ForegroundColor Yellow
                $script:FAIL++
            }
        }
    } else {
        Write-Host "  FAIL: Executable not produced" -ForegroundColor Red
        $script:FAIL++
    }
}

# Run all tests
Test-File "hello" "Hello World"
Test-File "math" ("30" + "`n" + "200" + "`n" + "10" + "`n" + "2")
Test-File "condition" "Greater"
Test-File "loop" ("1" + "`n" + "2" + "`n" + "3" + "`n" + "4" + "`n" + "5")
Test-File "class" "12"
Test-File "concat" "n=42"
Test-File "form" ("clicked" + "`n" + "hello")

# Cleanup
Get-ChildItem $TESTS_DIR -Filter "*.exe" | Remove-Item
Get-ChildItem $TESTS_DIR -Filter "*.obj" | Remove-Item
Get-ChildItem $TESTS_DIR -Filter "*.c" | Remove-Item

Write-Host ""
Write-Host "Results: $PASS passed, $FAIL failed" -ForegroundColor Cyan

if ($FAIL -gt 0) {
    exit 1
}
