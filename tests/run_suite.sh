#!/bin/sh
# Cross-platform test runner (Linux, macOS, and Git Bash on Windows).
#
#   tests/run_suite.sh [path/to/csec++]
#
# 1. fixtures: every tests/{positive,semantic_positive,stdlib,selfhost_concurrency} program is compiled
#    with --emit-ir and run with --run-ir; the pair of exit codes must match tests/expected_results.txt.
# 2. verify: tests/verify/*.csec through `csec++ --verify` must print their .expected diagnostics
#    (skipped when csec++ was built without Z3).
# 3. self-host: tests/selfhost_concurrency/*.csec compiled by the self-hosted compiler must exit 42.
#
# Set UPDATE=1 to rewrite tests/expected_results.txt from this run.
set -u
repo="$(cd "$(dirname "$0")/.." && pwd)"
cd "$repo"
compiler="${1:-}"
if [ -z "$compiler" ]; then
  for candidate in x64/Debug/csec++.exe x64/Release/csec++.exe Debug/csec++ Release/csec++; do
    if [ -x "$candidate" ]; then compiler="$candidate"; break; fi
  done
fi
[ -n "$compiler" ] && [ -x "$compiler" ] || { echo "csec++ not found; pass its path" >&2; exit 2; }

if command -v timeout >/dev/null 2>&1; then limit="timeout"
elif command -v gtimeout >/dev/null 2>&1; then limit="gtimeout"
else limit=""; fi
run_limited() {  # seconds, command...
  seconds="$1"; shift
  if [ -n "$limit" ]; then "$limit" "$seconds" "$@"; else "$@"; fi
}

work="$(mktemp -d 2>/dev/null || mktemp -d -t tessera)"
trap 'rm -rf "$work"' EXIT
failures=0

# ---- 1. fixtures ----
results="$work/results.txt"
: > "$results"
find tests/positive tests/semantic_positive tests/stdlib tests/selfhost_concurrency -name '*.csec' | LC_ALL=C sort | while read -r f; do
  out="$work/fixture.ll"
  rm -f "$out"
  run_limited 120 "$compiler" --emit-ir -o "$out" "$f" > "$work/emit.txt" 2>&1 < /dev/null
  e=$?
  r=NA
  if [ "$e" -eq 0 ] && [ -f "$out" ]; then
    run_limited 60 "$compiler" --run-ir "$out" > "$work/run.txt" 2>&1 < /dev/null
    r=$?
  fi
  echo "$f $e $r" >> "$results"
done
if [ "${UPDATE:-0}" = "1" ]; then
  cp "$results" tests/expected_results.txt
  echo "fixtures: wrote tests/expected_results.txt"
else
  if diff -u tests/expected_results.txt "$results" > "$work/fixtures.diff"; then
    echo "fixtures: $(wc -l < "$results" | tr -d ' ') matched"
  else
    echo "fixtures: MISMATCH (expected vs actual)"
    grep '^[-+]tests' "$work/fixtures.diff"
    failures=$((failures + 1))
  fi
fi

# ---- 2. verify ----
if "$compiler" --verify tests/verify/04_wrap_mode.csec 2>&1 | grep -q "built without Z3"; then
  echo "verify: skipped (csec++ was built without Z3)"
else
  passed=0; failed=0
  for f in tests/verify/*.csec; do
    flags="$(tr -d '\r' < "$f" | sed -n 's#^// verify-flags:[[:space:]]*##p' | head -n 1)"
    [ -n "$flags" ] || flags="--verify"
    # shellcheck disable=SC2086
    run_limited 300 "$compiler" $flags "$f" 2>&1 | tr -d '\r' |
      grep -v -e '^get current directory' -e '^Parsing completed' -e '^    ' > "$work/actual.txt"
    tr -d '\r' < "${f%.csec}.expected" | sed '1s/^\xEF\xBB\xBF//' > "$work/expected.txt"
    if diff -q "$work/expected.txt" "$work/actual.txt" > /dev/null; then
      passed=$((passed + 1))
    else
      failed=$((failed + 1))
      echo "verify: FAIL $f"
      diff -u "$work/expected.txt" "$work/actual.txt"
    fi
  done
  echo "verify: $passed passed, $failed failed"
  [ "$failed" -eq 0 ] || failures=$((failures + 1))
fi

# ---- 3. self-host ----
passed=0; failed=0
for f in tests/selfhost_concurrency/*.csec; do
  out="$work/selfhost.ll"
  rm -f "$out"
  run_limited 300 "$compiler" --run-ir selfhost/nativeflow_stage5_current.ll -- "$f" "$out" llvm > /dev/null 2>&1 < /dev/null
  code=NA
  if [ -f "$out" ]; then
    run_limited 60 "$compiler" --run-ir "$out" > /dev/null 2>&1 < /dev/null
    code=$?
  fi
  if [ "$code" = "42" ]; then passed=$((passed + 1)); else failed=$((failed + 1)); echo "self-host: FAIL $f (exit $code)"; fi
done
git checkout -- selfhost/last_llvm_helper.txt 2>/dev/null || true
echo "self-host: $passed passed, $failed failed"
[ "$failed" -eq 0 ] || failures=$((failures + 1))

[ "$failures" -eq 0 ] && echo "ALL PASSED" || echo "FAILED ($failures suite(s))"
[ "$failures" -eq 0 ]
