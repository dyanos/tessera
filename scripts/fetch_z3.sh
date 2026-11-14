#!/bin/sh
# Downloads the official Z3 release used by `csec++ --verify` into deps/z3 (macOS / Linux).
# Alternatively install Z3 with your package manager (brew install z3, apt install libz3-dev) and
# CMake finds it on the system paths.
set -eu
VERSION="${1:-5.3.3}"
repo="$(cd "$(dirname "$0")/.." && pwd)"
deps="$repo/deps"
case "$(uname -s)-$(uname -m)" in
  Darwin-arm64) name="z3-$VERSION-arm64-osx-13.3" ;;
  Darwin-x86_64) name="z3-$VERSION-x64-osx-13.3" ;;
  Linux-x86_64) name="z3-$VERSION-x64-glibc-2.39" ;;
  Linux-aarch64) name="z3-$VERSION-arm64-glibc-2.38" ;;
  *) echo "unsupported platform; install Z3 and set Z3_ROOT" >&2; exit 1 ;;
esac
[ -f "$deps/z3/include/z3++.h" ] && { echo "Z3 already in $deps/z3"; exit 0; }
mkdir -p "$deps"
curl -fL -o "$deps/$name.zip" "https://github.com/Z3Prover/z3/releases/download/z3-$VERSION/$name.zip"
# Compare with the release's published digest (GitHub release page / API "digest" field).
shasum -a 256 "$deps/$name.zip"
if command -v unzip >/dev/null 2>&1; then
  unzip -q -o "$deps/$name.zip" -d "$deps"
else
  python3 -m zipfile -e "$deps/$name.zip" "$deps"
fi
mv "$deps/$name" "$deps/z3"
rm "$deps/$name.zip"
echo "Z3 $VERSION is in $deps/z3"
