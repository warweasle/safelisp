#!/bin/bash
# Generic test harness for safelisp.
#
# Layout (a "suite" is a subdirectory; add new suites by adding new subdirs):
#   tests/cases/<suite>/<name>.safe        one top-level lisp form fed to ./safelisp on stdin
#   tests/expected/<suite>/<name>.expected  expected stdout, with the env-dump
#                                            lines main.c prints before/after
#                                            eval already stripped out
#
# Usage:
#   tests/run_tests.sh                 run every suite, compare against expected/
#   tests/run_tests.sh <suite>         run only that suite (e.g. "printer")
#   tests/run_tests.sh --bless         (re)generate expected/ from current actual output
#   tests/run_tests.sh --bless <suite> bless only that suite
#
# Always eyeball actual output for correctness before blessing it -- bless
# records whatever the binary currently does, it does not judge correctness.

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
BIN="$REPO_ROOT/safelisp"
CASES_DIR="$SCRIPT_DIR/cases"
EXPECTED_DIR="$SCRIPT_DIR/expected"

BLESS=0
SUITE_FILTER=""
for arg in "$@"; do
  case "$arg" in
    --bless) BLESS=1 ;;
    *) SUITE_FILTER="$arg" ;;
  esac
done

if [ ! -x "$BIN" ]; then
  echo "ERROR: $BIN not found or not executable. Build it first (make)." >&2
  exit 1
fi

# Strips the env-dump lines main.c prints before and after every eval.
# That line always contains both *INPUT* and *OUTPUT* markers, which real
# printed output never does, so filtering on those markers is safe.
filter_output() {
  grep -Fv -- '*INPUT*' | grep -Fv -- '*OUTPUT*'
}

pass=0
fail=0
blessed=0
nobaseline=0

shopt -s nullglob

for suite_dir in "$CASES_DIR"/*/; do
  suite="$(basename "$suite_dir")"
  if [ -n "$SUITE_FILTER" ] && [ "$suite" != "$SUITE_FILTER" ]; then
    continue
  fi

  for case_file in "$suite_dir"*.safe; do
    name="$(basename "$case_file" .safe)"
    expected_file="$EXPECTED_DIR/$suite/$name.expected"

    actual="$("$BIN" < "$case_file" 2>&1 | filter_output)"

    if [ "$BLESS" -eq 1 ]; then
      mkdir -p "$EXPECTED_DIR/$suite"
      printf '%s\n' "$actual" > "$expected_file"
      echo "BLESS   $suite/$name"
      blessed=$((blessed+1))
      continue
    fi

    if [ ! -f "$expected_file" ]; then
      echo "NOBASE  $suite/$name  (no expected file yet -- verify actual output, then: run_tests.sh --bless $suite)"
      nobaseline=$((nobaseline+1))
      continue
    fi

    expected="$(cat "$expected_file")"
    if [ "$actual" == "$expected" ]; then
      echo "PASS    $suite/$name"
      pass=$((pass+1))
    else
      echo "FAIL    $suite/$name"
      diff <(printf '%s\n' "$expected") <(printf '%s\n' "$actual") | sed 's/^/          /'
      fail=$((fail+1))
    fi
  done
done

echo
echo "pass=$pass fail=$fail nobaseline=$nobaseline blessed=$blessed"

[ "$fail" -eq 0 ]
