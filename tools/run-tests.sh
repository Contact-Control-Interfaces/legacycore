#!/usr/bin/env bash
# =============================================================================
# tools/run-tests.sh
# -----------------------------------------------------------------------------
# The Linux counterpart of tools/run-tests.ps1: runs the automated tests in
# test/unit with CTest and writes one JUnit XML report per executable,
# unit_tests.xml and api_tests.xml. Used by ./build.sh --tests and so by the
# build-linux job in CI.
#
#     tools/run-tests.sh                          # build/release-linux -> reports/test-results
#     tools/run-tests.sh <build-dir> <results-dir>
#
# The tests need no Service or glove: api_tests talk to a fake daemon on a Unix
# socket of their own (test/unit/support/fake_daemon_posix.*). Each GoogleTest
# case is its own CTest test, so the reports list every case, and a hung case is
# stopped by CTest's timeout instead of holding the job. Exit code 0 only if both
# executables ran, wrote their report, and every test passed (skipped tests are
# allowed).
# =============================================================================
set -uo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${1:-$root/build/release-linux}"
results_dir="${2:-$root/reports/test-results}"

for exe in unit_tests api_tests; do
    if [ ! -x "$build_dir/$exe" ]; then
        echo "run-tests.sh: $build_dir/$exe not found. Build it with: ./build.sh --tests" >&2
        exit 1
    fi
done

mkdir -p "$results_dir"
rm -f "$results_dir"/*.xml

status=0
for label in unit api; do
    report="$results_dir/${label}_tests.xml"
    echo
    echo "== ${label}_tests"
    # The label regex is anchored: "unit" must not also select "api".
    if ! ctest --test-dir "$build_dir" --label-regex "^${label}\$" --no-tests=error \
               --output-on-failure --timeout 60 --output-junit "$report"; then
        status=1
    fi
    if [ ! -s "$report" ]; then
        echo "run-tests.sh: ctest wrote no $report" >&2
        status=1
    fi
done

exit $status
