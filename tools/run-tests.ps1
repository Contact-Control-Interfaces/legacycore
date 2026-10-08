#Requires -Version 5.1
<#
    run-tests.ps1
    ---------------------------------------------------------------
    Runs the automated tests (unit_tests.exe, api_tests.exe) and writes one
    JUnit XML report per executable. Used by build.ps1 -IncludeTests and by
    the `test` job in CI, which runs it against the binaries the `build`
    job produced - so the libcci.dll / libccic.dll under test are the ones
    that ship.

        .\tools\run-tests.ps1                         # build\release-mingw -> reports\test-results
        .\tools\run-tests.ps1 -BinDir <dir> -ResultsDir <dir>

    The test executables are linked statically and need nothing but the two
    DLLs next to them. Exit code 0 only if every executable ran, wrote its
    report, and every test passed (skipped tests are allowed).
#>

[CmdletBinding()]
param(
    [string]$BinDir,
    [string]$ResultsDir
)

$ErrorActionPreference = 'Stop'

$Root = Split-Path -Parent $PSScriptRoot
if (-not $BinDir)     { $BinDir     = Join-Path $Root 'build\release-mingw' }
if (-not $ResultsDir) { $ResultsDir = Join-Path $Root 'reports\test-results' }

New-Item -ItemType Directory -Force -Path $ResultsDir | Out-Null
Get-ChildItem $ResultsDir -Filter *.xml -ErrorAction SilentlyContinue | Remove-Item -Force

$failures = @()
$totals = @{ tests = 0; failed = 0; skipped = 0 }

foreach ($name in 'unit_tests', 'api_tests') {
    $exe = Join-Path $BinDir "$name.exe"
    if (-not (Test-Path $exe)) { throw "$exe not found. Build with: .\build.ps1 -IncludeTests" }
    $report = Join-Path $ResultsDir "$name.xml"

    Write-Host ""
    Write-Host "== $name" -ForegroundColor Cyan
    & $exe "--gtest_output=xml:$report"
    $exit = $LASTEXITCODE

    if (-not (Test-Path $report)) {
        # GoogleTest writes the report when the run ends; none means the process died.
        $failures += "$name exited $exit without writing $report (crashed?)"
        continue
    }
    $suites  = ([xml](Get-Content $report -Raw)).testsuites
    $tests   = [int]$suites.tests
    $failed  = [int]$suites.failures + [int]$suites.errors
    $skipped = @($suites.SelectNodes('//testcase[@result="skipped"]')).Count
    $totals.tests += $tests; $totals.failed += $failed; $totals.skipped += $skipped
    Write-Host "   $name`: $tests test(s), $($tests - $failed - $skipped) passed, $failed failed, $skipped skipped -> $report"

    if ($tests -eq 0)  { $failures += "$name ran no tests" }
    if ($exit -ne 0)   { $failures += "$name exited $exit" }
}

Write-Host ""
Write-Host ("Total: {0} test(s), {1} passed, {2} failed, {3} skipped" -f
            $totals.tests, ($totals.tests - $totals.failed - $totals.skipped), $totals.failed, $totals.skipped)
if ($failures.Count -gt 0) {
    $failures | ForEach-Object { Write-Host "FAILED: $_" -ForegroundColor Red }
    exit 1
}
exit 0
