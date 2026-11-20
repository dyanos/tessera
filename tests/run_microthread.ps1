param(
    [string]$Filter = "*",
    [int]$TimeoutSeconds = 30,
    [ValidateSet("microthread", "stdlib")]
    [string]$Suite = "microthread",
    # Compiler to test (default: the Debug build). E.g. x64\Release\tess.exe for another LLVM build.
    [string]$Compiler = ""
)

# Microthreads/async (tests/microthread) and the standard library (tests/stdlib, -Suite stdlib).
# Positives must compile and run to exit code 0; negatives must be rejected by the compiler.

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$exe = Join-Path $repoRoot "x64\Debug\tess.exe"
if ($Compiler) { $exe = (Resolve-Path $Compiler).Path }
$root = Join-Path $PSScriptRoot $Suite

if (-not (Test-Path $exe)) {
    Write-Error "Compiler executable not found: $exe"
}

function Invoke-Csec([string]$arguments) {
    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = $exe
    $psi.Arguments = $arguments
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
        return 124
    }
    $process.WaitForExit()
    $null = $stdoutTask.GetAwaiter().GetResult()
    $null = $stderrTask.GetAwaiter().GetResult()
    return $process.ExitCode
}

$failures = 0
$passed = 0

foreach ($file in Get-ChildItem (Join-Path $root "positive") -Filter "$Filter.csec" | Sort-Object Name) {
    $ll = [System.IO.Path]::GetTempFileName() + ".ll"
    $emit = Invoke-Csec "`"$($file.FullName)`" --emit-ir -o `"$ll`""
    $run = if ($emit -eq 0) { Invoke-Csec "--run-ir `"$ll`"" } else { -1 }
    Remove-Item -Force -ErrorAction SilentlyContinue $ll
    if ($emit -eq 0 -and $run -eq 0) {
        $passed++
    } else {
        $failures++
        Write-Host "FAIL positive $($file.Name): emit=$emit run=$run"
    }
}

foreach ($file in Get-ChildItem (Join-Path $root "negative") -Filter "$Filter.csec" -ErrorAction SilentlyContinue | Sort-Object Name) {
    $ll = [System.IO.Path]::GetTempFileName() + ".ll"
    $emit = Invoke-Csec "`"$($file.FullName)`" --emit-ir -o `"$ll`""
    Remove-Item -Force -ErrorAction SilentlyContinue $ll
    if ($emit -ne 0) {
        $passed++
    } else {
        $failures++
        Write-Host "FAIL negative $($file.Name): compiled but should have been rejected"
    }
}

Write-Host "${Suite}: $passed passed, $failures failed"
if ($failures -gt 0) { exit 1 }
