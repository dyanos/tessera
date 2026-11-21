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
work="$(mktemp -d 2>/dev/null || mktemp -d -t tessera)" || exit 2
trap 'rm -rf "$work"' EXIT
failures=0
checked=0
if [ "$#" -gt 0 ]; then shift; fi
if [ "$#" -gt 0 ]; then
  printf '%s\n' "$@" > "$work/cases"
else
  find tests/positive tests/semantic_positive tests/stdlib/positive tests/selfhost_concurrency -name '*.csec' > "$work/unsorted" || exit 2
  LC_ALL=C sort "$work/unsorted" > "$work/cases" || exit 2
fi
tr -d '\r' < tests/leak_check_exempt.txt > "$work/exempt" || exit 2
while IFS= read -r f; do
  if grep -Fqx -- "$f" "$work/exempt"; then
    printf '  exempt: %s\n' "$f"
    continue
  fi
  : > "$work/p.ll"
  if ! "$compiler" --leak-check --emit-ir -o "$work/p.ll" "$f" > "$work/out.txt" 2>&1 < /dev/null || [ ! -s "$work/p.ll" ]; then
    printf '  compile failed: %s\n' "$f"
    cat "$work/out.txt"
    failures=$((failures + 1))
    continue
  fi
  LC_ALL=C CSEC_LEAK_CHECK=1 timeout --verbose --kill-after=5 60 "$compiler" --run-ir "$work/p.ll" > "$work/out.txt" 2>&1 < /dev/null
  status=$?
  line="$(grep -aE '^csec-leak-check: [0-9]+ block\(s\), [0-9]+ byte\(s\) not freed' "$work/out.txt" | tail -1)"
  if [ -z "$line" ] || grep -aqE '^timeout: |^Input LLVM IR (has no executable entry function|is invalid)\.' "$work/out.txt"; then
    printf '  execution failed or leak report missing: %s (exit %s)\n' "$f" "$status"
    cat "$work/out.txt"
    failures=$((failures + 1))
    continue
  fi
  checked=$((checked + 1))
  blocks="$(echo "$line" | awk '{print $2}')"
  if [ "$blocks" != "0" ]; then
    echo "  leak: $f ($line)"
    failures=$((failures + 1))
  fi
done < "$work/cases"
echo "leak check: $checked program(s) checked, $failures failure(s)"
[ "$checked" -gt 0 ] && [ "$failures" -eq 0 ]
