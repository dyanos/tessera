param(
    [string]$Filter = "*",
    [int]$TimeoutSeconds = 120,
    # Compiler to test (default: the Debug build). E.g. x64\Release\tess.exe for another LLVM build.
    [string]$Compiler = ""
)

# Concurrency and standard-library features through BOTH compilers. Each fixture in
# tests/selfhost_concurrency must exit with 42 when compiled by the native compiler (--emit-ir) and
# by the self-hosted compiler (selfhost/nativeflow_stage5_current.ll run under --run-ir).

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$exe = Join-Path $repoRoot "x64\Debug\tess.exe"
if ($Compiler) { $exe = (Resolve-Path $Compiler).Path }
$stage = Join-Path $repoRoot "selfhost\nativeflow_stage5_current.ll"
$root = Join-Path $PSScriptRoot "selfhost_concurrency"

foreach ($required in @($exe, $stage)) {
    if (-not (Test-Path $required)) { Write-Error "Missing: $required" }
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
foreach ($file in Get-ChildItem $root -Filter "$Filter.csec" | Sort-Object Name) {
    $nativeIr = [System.IO.Path]::GetTempFileName() + ".ll"
    $selfhostIr = [System.IO.Path]::GetTempFileName() + ".ll"

    $emit = Invoke-Csec "`"$($file.FullName)`" --emit-ir -o `"$nativeIr`""
    $native = if ($emit -eq 0) { Invoke-Csec "--run-ir `"$nativeIr`"" } else { -1 }

    $compile = Invoke-Csec "--run-ir `"$stage`" -- `"$($file.FullName)`" `"$selfhostIr`" llvm"
    $selfhost = if ($compile -eq 0 -and (Test-Path $selfhostIr)) { Invoke-Csec "--run-ir `"$selfhostIr`"" } else { -1 }

    Remove-Item -Force -ErrorAction SilentlyContinue $nativeIr, $selfhostIr
    if ($native -eq 42 -and $selfhost -eq 42) {
        $passed++
    } else {
        $failures++
        Write-Host "FAIL $($file.Name): native=$native selfhost=$selfhost"
    }
}

Write-Host "selfhost_concurrency: $passed passed, $failures failed"
if ($failures -gt 0) { exit 1 }
