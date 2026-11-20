param(
    [string]$Filter = "*",
    [switch]$StopOnFailure,
    [switch]$Quiet,
    [int]$TimeoutSeconds = 10
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$exe = Join-Path $repoRoot "x64\Debug\tess.exe"

if (-not (Test-Path $exe)) {
    Write-Error "Compiler executable not found: $exe"
}

$files = Get-ChildItem -Path (Join-Path $PSScriptRoot "negative") -Recurse -Filter *.csec |
    Where-Object { $_.Name -like $Filter -or $_.FullName -like "*$Filter*" } |
    Sort-Object FullName

if ($files.Count -eq 0) {
    Write-Error "No test files matched filter '$Filter'"
}

$results = New-Object System.Collections.Generic.List[object]
$passed = 0
$failed = 0

foreach ($file in $files) {
    if (-not $Quiet) {
        Write-Host "=== $($file.FullName) ==="
    }

    $output = @()
    $timedOut = $false
    try {
        $psi = New-Object System.Diagnostics.ProcessStartInfo
        $psi.FileName = $exe
        $psi.Arguments = "--syntax-only `"$($file.FullName)`""
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
            $timedOut = $true
            try {
                $process.Kill()
            }
            catch {
            }
        }
        $process.WaitForExit()
        $stdout = $stdoutTask.GetAwaiter().GetResult()
        $stderr = $stderrTask.GetAwaiter().GetResult()

        if ($stdout) {
            $output += ($stdout -split "`r?`n" | Where-Object { $_ -ne "" })
        }
        if ($stderr) {
            $output += ($stderr -split "`r?`n" | Where-Object { $_ -ne "" })
        }

        if ($timedOut) {
            $output += "Timed out after $TimeoutSeconds seconds"
            $exitCode = 124
        }
        else {
            $exitCode = $process.ExitCode
        }
    }
    catch {
        $output = @($_.Exception.Message)
        $exitCode = 1
    }

    $isPass = $exitCode -ne 0
    if ($isPass) {
        $passed++
    }
    else {
        $failed++
    }

    $results.Add([pscustomobject]@{
        File = $file.FullName
        ExitCode = $exitCode
        Passed = $isPass
        Output = $output
        TimedOut = $timedOut
    }) | Out-Null

    if (-not $Quiet) {
        if ($output.Count -gt 0) {
            $output | ForEach-Object { Write-Host $_ }
        }
        Write-Host ""
    }

    if ($StopOnFailure -and -not $isPass) {
        break
    }
}

Write-Host "Total: $($results.Count)"
Write-Host "Passed: $passed"
Write-Host "Failed: $failed"

if ($failed -gt 0) {
    Write-Host ""
    Write-Host "Unexpected successes:"
    foreach ($result in $results | Where-Object { -not $_.Passed }) {
        Write-Host "- $($result.File) [exit=$($result.ExitCode)]"
        $result.Output | Select-Object -First 3 | ForEach-Object { Write-Host "  $_" }
    }

    exit 1
}
