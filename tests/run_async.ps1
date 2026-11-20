param(
    [string]$Filter = "*",
    [switch]$Quiet,
    [int]$TimeoutSeconds = 10
)

# Exercises the async/await surface and the M8 no-borrow-across-await rule. Positives must compile;
# negatives (await outside async, borrow held across await) must be rejected by the type checker.

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$exe = Join-Path $repoRoot "x64\Debug\tess.exe"
$root = Join-Path $PSScriptRoot "async"

if (-not (Test-Path $exe)) {
    Write-Error "Compiler executable not found: $exe"
}

function Invoke-Compile([string]$file) {
    $outFile = [System.IO.Path]::GetTempFileName() + ".ll"
    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = $exe
    $psi.Arguments = "`"$file`" --emit-ir -o `"$outFile`""
    $psi.WorkingDirectory = $repoRoot
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $process = New-Object System.Diagnostics.Process
    $process.StartInfo = $psi
    $null = $process.Start()
    $stdoutTask = $process.StandardOutput.ReadToEndAsync()
    $stderrTask = $process.StandardError.ReadToEndAsync()
    if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
        try { $process.Kill() } catch {}
        Remove-Item -Force -ErrorAction SilentlyContinue $outFile
        return @{ ExitCode = 124 }
    }
    $process.WaitForExit()
    $null = $stdoutTask.GetAwaiter().GetResult()
    $null = $stderrTask.GetAwaiter().GetResult()
    Remove-Item -Force -ErrorAction SilentlyContinue $outFile
    return @{ ExitCode = $process.ExitCode }
}

$passed = 0
$failed = 0
$failures = New-Object System.Collections.Generic.List[string]

foreach ($kind in @(@{ Dir = "positive"; ExpectSuccess = $true }, @{ Dir = "negative"; ExpectSuccess = $false })) {
    $dir = Join-Path $root $kind.Dir
    if (-not (Test-Path $dir)) { continue }
    $files = Get-ChildItem -Path $dir -Recurse -Filter *.csec |
        Where-Object { $_.Name -like $Filter -or $_.FullName -like "*$Filter*" } |
        Sort-Object FullName
    foreach ($file in $files) {
        $result = Invoke-Compile $file.FullName
        $compiled = ($result.ExitCode -eq 0)
        $isPass = ($compiled -eq $kind.ExpectSuccess)
        if ($isPass) {
            $passed++
            if (-not $Quiet) { Write-Host "PASS [$($kind.Dir)] $($file.Name)" }
        }
        else {
            $failed++
            $expectTxt = if ($kind.ExpectSuccess) { "compile" } else { "be rejected" }
            $failures.Add("[$($kind.Dir)] $($file.Name) — expected to $expectTxt (exit=$($result.ExitCode))")
            if (-not $Quiet) { Write-Host "FAIL [$($kind.Dir)] $($file.Name) [exit=$($result.ExitCode)]" }
        }
    }
}

Write-Host ""
Write-Host "Total: $($passed + $failed)"
Write-Host "Passed: $passed"
Write-Host "Failed: $failed"

if ($failed -gt 0) {
    Write-Host ""
    Write-Host "Failures:"
    foreach ($f in $failures) { Write-Host "- $f" }
    exit 1
}
