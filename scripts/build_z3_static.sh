#!/bin/sh
# Builds Z3 from source as a static library into deps/z3-static (include/ and lib/), for
# `cmake -DCSEC_Z3_LINK=STATIC` on Linux and macOS. The official releases only ship the shared
# library, so the static one is compiled here (this takes a while). The code is position-independent
# because it is also linked into the System.Native shared runtime.
set -eu
VERSION="${1:-5.3.3}"
repo="$(cd "$(dirname "$0")/.." && pwd)"
deps="$repo/deps"
source="$deps/z3-src-$VERSION"
build="$deps/z3-static-build"
install="$deps/z3-static"
mkdir -p "$deps"
if [ ! -f "$source/CMakeLists.txt" ]; then
  git clone --depth 1 --branch "z3-$VERSION" https://github.com/Z3Prover/z3.git "$source"
fi
generator=""
command -v ninja >/dev/null 2>&1 && generator="-G Ninja"
# shellcheck disable=SC2086
cmake -S "$source" -B "$build" $generator \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
  -DZ3_BUILD_LIBZ3_SHARED=OFF \
  -DZ3_BUILD_EXECUTABLE=OFF \
  -DZ3_BUILD_TEST_EXECUTABLES=OFF \
  -DZ3_BUILD_PYTHON_BINDINGS=OFF \
  -DZ3_ENABLE_EXAMPLE_TARGETS=OFF \
  -DCMAKE_INSTALL_PREFIX="$install"
cmake --build "$build" --target install --parallel
echo "Static Z3 $VERSION is in $install"
echo "Configure tessera with -DCSEC_Z3_LINK=STATIC"
