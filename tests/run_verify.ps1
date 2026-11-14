param(
    [string]$Filter = "*",
    [switch]$Update
)
# Runs `csec++ --verify` on tests/verify/*.csec and compares its diagnostics with the .expected file
# next to each fixture (counterexample lines are skipped: models are not unique). A fixture may give
# its verifier flags on a line `// verify-flags: ...`. -Update rewrites the .expected files.
$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$exe = Join-Path $repoRoot "x64\Debug\csec++.exe"
$passed = 0
$failed = 0
foreach ($file in Get-ChildItem (Join-Path $PSScriptRoot "verify") -Filter "$Filter.csec" | Sort-Object Name) {
    $flags = "--verify"
    $flagLine = Select-String -Path $file.FullName -Pattern "^// verify-flags:\s*(.+)$" | Select-Object -First 1
    if ($flagLine) { $flags = $flagLine.Matches[0].Groups[1].Value.Trim() }
    $output = & $exe $flags.Split(" ") $file.FullName 2>&1 | ForEach-Object { "$_" } |
        Where-Object { $_ -notmatch "^get current directory" -and $_ -notmatch "^Parsing completed" -and $_ -notmatch "^    " }
    $actual = ($output -join "`n").Trim()
    $expectedFile = [System.IO.Path]::ChangeExtension($file.FullName, ".expected")
    if ($Update) {
        Set-Content -Path $expectedFile -Value $actual -Encoding utf8
        Write-Host "updated $($file.Name)"
        continue
    }
    $expected = if (Test-Path $expectedFile) { (Get-Content $expectedFile -Raw).Trim() -replace "`r`n", "`n" } else { "<missing .expected>" }
    if ($actual -eq $expected) {
        $passed++
    } else {
        $failed++
        Write-Host "FAIL $($file.Name)"
        Write-Host "--- expected"; Write-Host $expected
        Write-Host "--- actual"; Write-Host $actual
    }
}
Write-Host "verify: $passed passed, $failed failed"
if ($failed -gt 0) { exit 1 }
