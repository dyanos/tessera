# Builds Z3 from source as a static library into deps/z3-static (include/ and lib/), for
# `cmake -DCSEC_Z3_LINK=STATIC`. The official releases only ship libz3.dll, so the static library is
# compiled here (this takes a while). Run it from a Visual Studio developer prompt.
#
# MSVC requires the static library and csec++ to use the same C runtime: -Config Debug (the default,
# /MDd, for the Debug csec++) or -Config Release (/MT, or /MD with -DllCrt to match
# -DCSEC_RELEASE_DLL_CRT=ON). Rebuild after switching.
param(
    [string]$Version = "5.3.3",
    [ValidateSet("Debug", "Release")]
    [string]$Config = "Debug",
    [switch]$DllCrt
)
$ErrorActionPreference = "Stop"
$repo = Split-Path -Parent $PSScriptRoot
$deps = Join-Path $repo "deps"
$source = Join-Path $deps "z3-src-$Version"
$build = Join-Path $deps "z3-static-build-$Config"
$install = Join-Path $deps "z3-static"
New-Item -ItemType Directory -Force $deps | Out-Null

if (-not (Test-Path (Join-Path $source "src/api/z3.h"))) {
    if (Test-Path $source) { Remove-Item -Recurse -Force $source }
    # Z3's test tree has paths longer than Windows' default limit.
    git -c core.longpaths=true clone --depth 1 --branch "z3-$Version" https://github.com/Z3Prover/z3.git $source
    if ($LASTEXITCODE -ne 0) { throw "git clone of Z3 $Version failed" }
}

$crt = if ($Config -eq "Debug") { "MultiThreadedDebugDLL" } elseif ($DllCrt) { "MultiThreadedDLL" } else { "MultiThreaded" }
cmake -S $source -B $build -G Ninja `
    "-DCMAKE_BUILD_TYPE=$Config" `
    "-DCMAKE_POLICY_DEFAULT_CMP0091=NEW" `
    "-DCMAKE_MSVC_RUNTIME_LIBRARY=$crt" `
    "-DZ3_BUILD_LIBZ3_SHARED=OFF" `
    "-DZ3_BUILD_EXECUTABLE=OFF" `
    "-DZ3_BUILD_TEST_EXECUTABLES=OFF" `
    "-DZ3_BUILD_PYTHON_BINDINGS=OFF" `
    "-DZ3_ENABLE_EXAMPLE_TARGETS=OFF" `
    "-DCMAKE_INSTALL_PREFIX=$install"
if ($LASTEXITCODE -ne 0) { throw "configuring Z3 failed" }
cmake --build $build --target install --parallel
if ($LASTEXITCODE -ne 0) { throw "building Z3 failed" }
Set-Content -Path (Join-Path $install "CSEC_Z3_STATIC_CONFIG.txt") -Value "$Version $Config $crt"
Write-Host "Static Z3 $Version ($Config, $crt) is in $install"
Write-Host "Configure tessera with -DCSEC_Z3_LINK=STATIC"
