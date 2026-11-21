param([string]$Compiler = "")

$repo = Split-Path -Parent $PSScriptRoot
$compilerPath = if ($Compiler) { (Resolve-Path -LiteralPath $Compiler).Path } else { Join-Path $repo "x64\Debug\tess.exe" }
$source = Join-Path $repo "selfhost\src\compiler.csec"
$fixture = Join-Path $PSScriptRoot "positive\11_tensor\129_selfhost_tensor_scope.csec"
$scratch = Join-Path ([IO.Path]::GetTempPath()) ([guid]::NewGuid().ToString())
$stage = "$scratch-stage.ll"
$program = "$scratch-program.ll"
$log = "$scratch-leaks.txt"
$previousLeakCheck = $env:CSEC_LEAK_CHECK

Push-Location $repo
try {
    $env:CSEC_LEAK_CHECK = $null
    & $compilerPath --emit-ir -o $stage $source
    if ($LASTEXITCODE -ne 0) { throw "Selfhost compiler build failed ($LASTEXITCODE)" }
    & $compilerPath --run-ir $stage -- $fixture $program llvm
    if ($LASTEXITCODE -ne 0) { throw "Selfhost tensor compilation failed ($LASTEXITCODE)" }
    $env:CSEC_LEAK_CHECK = "1"
    & $compilerPath --run-ir $program 2>$log
    if ($LASTEXITCODE -ne 0) { throw "Selfhost tensor program failed ($LASTEXITCODE)" }
    $report = Get-Content -LiteralPath $log -Raw
    if ($report -notmatch 'csec-leak-check: 0 block\(s\), 0 byte\(s\) not freed') { throw "Unexpected leak report: $report" }
    Write-Host "Selfhost tensor scope: passed with zero tracked leaks"
} finally {
    $env:CSEC_LEAK_CHECK = $previousLeakCheck
    Remove-Item -LiteralPath $stage, $program, $log -ErrorAction SilentlyContinue
    Pop-Location
}
