#!/bin/sh
set -u
repo="$(cd "$(dirname "$0")/.." && pwd)"
cd "$repo"
compiler="${1:-}"
if [ -z "$compiler" ]; then
  for candidate in x64/Debug/tess.exe x64/Release/tess.exe Debug/tess Release/tess; do
    if [ -x "$candidate" ]; then compiler="$candidate"; break; fi
  done
fi
[ -n "$compiler" ] && [ -x "$compiler" ] || { echo "tess not found; pass its path" >&2; exit 2; }
export CSEC_LIB_PATH="$repo/lib"
work="$(mktemp -d 2>/dev/null || mktemp -d -t tessera)"
trap 'rm -rf "$work"' EXIT
exempt="$(tr -d '\r' < tests/leak_check_exempt.txt 2>/dev/null | grep -v '^#')"
failures=0
checked=0
for f in $(find tests/positive tests/semantic_positive tests/stdlib tests/selfhost_concurrency -name '*.csec' | LC_ALL=C sort); do
  "$compiler" --leak-check --emit-ir -o "$work/p.ll" "$f" > /dev/null 2>&1 < /dev/null || continue
  CSEC_LEAK_CHECK=1 timeout 60 "$compiler" --run-ir "$work/p.ll" > "$work/out.txt" 2>&1 < /dev/null
  line="$(grep -a 'csec-leak-check:' "$work/out.txt" | tail -1)"
  [ -n "$line" ] || continue
  checked=$((checked + 1))
  blocks="$(echo "$line" | awk '{print $2}')"
  if [ "$blocks" != "0" ] && ! echo "$exempt" | grep -qx "$f"; then
    echo "  leak: $f ($line)"
    failures=$((failures + 1))
  fi
done
echo "leak check: $checked program(s), $failures leaking"
[ "$failures" -eq 0 ]
