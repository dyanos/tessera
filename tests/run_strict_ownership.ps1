param(
    [string]$Filter = "*",
    [switch]$Quiet,
    [int]$TimeoutSeconds = 10
)

# Exercises the opt-in M2 strict-ownership mode (`--strict-ownership`). Positives must compile with
# the flag on; negatives must be rejected by the move-checker. Off-by-default behaviour (the rest of
# the corpus) is covered by run_positive / run_semantic_positive, which never pass the flag.

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$exe = Join-Path $repoRoot "x64\Debug\tess.exe"
$root = Join-Path $PSScriptRoot "strict_ownership"

if (-not (Test-Path $exe)) {
    Write-Error "Compiler executable not found: $exe"
}

function Invoke-Compile([string]$file) {
    $outFile = [System.IO.Path]::GetTempFileName() + ".ll"
    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = $exe
    $psi.Arguments = "`"$file`" --strict-ownership --emit-ir -o `"$outFile`""
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
        return @{ ExitCode = 124; Output = "Timed out" }
    }
    $process.WaitForExit()
    $err = $stderrTask.GetAwaiter().GetResult()
    Remove-Item -Force -ErrorAction SilentlyContinue $outFile
    return @{ ExitCode = $process.ExitCode; Output = $err }
}

$passed = 0
$failed = 0
$failures = New-Object System.Collections.Generic.List[string]

# expectSuccess = $true for positive/, $false for negative/
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
