param(
    [string]$Compiler = ".\x64\Debug\tess.exe",
    [string]$Clang = "C:\Program Files\LLVM\bin\clang.exe",
    [string]$RuntimeLib = ""
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$compilerPath = Join-Path $repoRoot $Compiler
$selfhostSource = Join-Path $PSScriptRoot "csec_compiler.csec"
$inputSource = Join-Path $PSScriptRoot "input.csec"
$llvmSubsetSource = Join-Path $PSScriptRoot "llvm_subset.csec"
$binDir = Join-Path $repoRoot "x64\Debug"
$selfhostExe = Join-Path $binDir "csec_selfhost_bootstrap.exe"
$bootInputLl = Join-Path $PSScriptRoot "boot_input.ll"
$bootSubsetAst = Join-Path $PSScriptRoot "boot_subset.ast"
$bootSubsetLl = Join-Path $PSScriptRoot "boot_subset.ll"
$bootSubsetExe = Join-Path $binDir "csec_selfhost_subset.exe"
$selfhostSourceRoot = Join-Path $PSScriptRoot "src"
$selfhostImportEntry = Join-Path $selfhostSourceRoot "compiler.csec"
$bootParserAst = Join-Path $PSScriptRoot "boot_parser.ast"
$bootIrgenAst = Join-Path $PSScriptRoot "boot_irgen.ast"
$bootSelfhostTokenLen = Join-Path $PSScriptRoot "boot_selfhost.tokenlen"
$bootImportEntryAst = Join-Path $PSScriptRoot "boot_import_entry.ast"

if (-not (Test-Path $compilerPath)) {
    Write-Error "Compiler not found: $compilerPath"
}
if (-not (Test-Path $Clang)) {
    Write-Error "Clang not found: $Clang"
}
$runtimeLibPath = $RuntimeLib
if ($runtimeLibPath.Length -gt 0 -and -not [System.IO.Path]::IsPathRooted($runtimeLibPath)) {
    $runtimeLibPath = Join-Path $repoRoot $runtimeLibPath
}
if ($runtimeLibPath.Length -gt 0 -and -not (Test-Path $runtimeLibPath)) {
    Write-Error "Runtime library not found: $runtimeLibPath"
}

powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "verify_selfhost.ps1") -Compiler $Compiler

New-Item -ItemType Directory -Path $binDir -Force | Out-Null

function Invoke-Checked {
    param(
        [string]$FilePath,
        [string[]]$Arguments,
        [int]$ExpectedExitCode = 0
    )

    $output = & $FilePath @Arguments 2>&1 | Out-String
    $exitCode = $LASTEXITCODE
    if ($output.Length -gt 0) {
        Write-Host $output.TrimEnd()
    }
    if ($exitCode -ne $ExpectedExitCode) {
        Write-Error "Command failed with exit code $exitCode, expected $ExpectedExitCode`: $FilePath $($Arguments -join ' ')"
    }
}

function Invoke-CheckedWithRetry {
    param(
        [string]$FilePath,
        [string[]]$Arguments,
        [int]$ExpectedExitCode = 0
    )

    try {
        Invoke-Checked $FilePath $Arguments $ExpectedExitCode
    }
    catch {
        Start-Sleep -Seconds 2
        Invoke-Checked $FilePath $Arguments $ExpectedExitCode
    }
}

Invoke-Checked $compilerPath @("--emit-exe", $selfhostSource, "-o", $selfhostExe)
Invoke-CheckedWithRetry $selfhostExe @($inputSource, $bootInputLl, "llvm")
Invoke-CheckedWithRetry $selfhostExe @($llvmSubsetSource, $bootSubsetAst, "ast")
Invoke-CheckedWithRetry $selfhostExe @($llvmSubsetSource, $bootSubsetLl, "llvm")
Invoke-CheckedWithRetry $selfhostExe @((Join-Path $selfhostSourceRoot "parser.csec"), $bootParserAst, "ast")
Invoke-CheckedWithRetry $selfhostExe @((Join-Path $selfhostSourceRoot "ir_generator.csec"), $bootIrgenAst, "ast")
Invoke-CheckedWithRetry $selfhostExe @($selfhostSource, $bootSelfhostTokenLen, "tokenlen")
Invoke-CheckedWithRetry $selfhostExe @($selfhostImportEntry, $bootImportEntryAst, "ast")

$bootInput = Get-Content $bootInputLl -Raw
$bootAst = Get-Content $bootSubsetAst -Raw
$bootSubset = Get-Content $bootSubsetLl -Raw
$bootParserAstText = Get-Content $bootParserAst -Raw
$bootIrgenAstText = Get-Content $bootIrgenAst -Raw
$bootSelfhostTokenLenText = Get-Content $bootSelfhostTokenLen -Raw
$bootImportEntryAstText = Get-Content $bootImportEntryAst -Raw

$requiredBootContracts = @(
    "; ModuleID = 'csec.selfhost'",
    "declare void @csec_set_command_line_args(i32, ptr)",
    "define i32 @main(i32 %argc, ptr %argv)",
    "define i32 @adjust",
    "define double @ratio",
    "for.cond.",
    "call i1 @positive",
    "call double @ratio"
)

foreach ($needle in $requiredBootContracts) {
    if (-not ($bootInput.Contains($needle) -or $bootSubset.Contains($needle))) {
        Write-Error "Missing bootstrap LLVM contract: $needle"
    }
}

foreach ($needle in @("Program", "Decl function ratio", "Stmt for", "Expr call target=ratio")) {
    if (-not $bootAst.Contains($needle)) {
        Write-Error "Missing bootstrap AST contract: $needle"
    }
}

foreach ($needle in @("Decl function generateAST", "Decl function generateSymbolTable")) {
    if (-not $bootParserAstText.Contains($needle)) {
        Write-Error "Missing parser selfhost AST contract: $needle"
    }
}

foreach ($needle in @("Decl function generateLLVMModule", "Decl function generateExecutionC")) {
    if (-not $bootIrgenAstText.Contains($needle)) {
        Write-Error "Missing IR generator selfhost AST contract: $needle"
    }
}

if (-not $bootSelfhostTokenLenText.StartsWith("tokens=")) {
    Write-Error "Missing aggregate selfhost token length output."
}

foreach ($needle in @("Decl function tokenize", "Decl function generateLLVMModule", "Decl function compileFile")) {
    if (-not $bootImportEntryAstText.Contains($needle)) {
        Write-Error "Missing imported selfhost AST contract: $needle"
    }
}

$clangArgs = @("-Wno-override-module", $bootSubsetLl)
if ($runtimeLibPath.Length -gt 0) {
    $clangArgs += $runtimeLibPath
}
$clangArgs += @("-o", $bootSubsetExe)
Invoke-Checked $Clang $clangArgs
Invoke-CheckedWithRetry $bootSubsetExe @() 10

Write-Host "Native selfhost bootstrap verification passed."
