# Downloads the official Z3 release used by `csec++ --verify` into deps/z3 (include/ and bin/),
# checks its SHA-256, and unpacks it. Re-run CMake afterwards so it finds deps/z3.
param(
    [string]$Version = "5.3.3",
    [string]$Sha256 = "4e48624a6895b49606b3cf91ca4fbcc069fb2973b9706532170a144baa3fd798"
)
$ErrorActionPreference = "Stop"
$repo = Split-Path -Parent $PSScriptRoot
$deps = Join-Path $repo "deps"
$target = Join-Path $deps "z3"
if (Test-Path (Join-Path $target "include\z3++.h")) { Write-Host "Z3 already in $target"; exit 0 }
New-Item -ItemType Directory -Force $deps | Out-Null
$name = "z3-$Version-x64-win"
$zip = Join-Path $deps "$name.zip"
$url = "https://github.com/Z3Prover/z3/releases/download/z3-$Version/$name.zip"
Write-Host "Downloading $url"
Invoke-WebRequest -Uri $url -OutFile $zip
$actual = (Get-FileHash -Algorithm SHA256 $zip).Hash.ToLower()
if ($actual -ne $Sha256.ToLower()) { Remove-Item $zip; throw "SHA-256 mismatch for $name.zip: $actual" }
$extract = Join-Path $deps "z3-extract"
Expand-Archive -Path $zip -DestinationPath $extract -Force
Move-Item (Join-Path $extract $name) $target
Remove-Item -Recurse -Force $extract, $zip
Write-Host "Z3 $Version is in $target"
