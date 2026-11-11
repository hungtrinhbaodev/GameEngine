#!/bin/bash
# Build ui_test and run every repro, stress, the scripted drive and a set of fuzz seeds.
# One summary line per case; exit code = number of failed cases.
# Usage: tests/run_all.sh            (from anywhere; logs go to build/ui_test_logs)
#        UI_TEST_SKIP_BUILD=1 tests/run_all.sh
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$ROOT/build/ui_test_logs"
mkdir -p "$OUT"
rm -f "$OUT"/*.log
cd "$ROOT" || exit 1
if [ -z "$UI_TEST_SKIP_BUILD" ]; then
  if ! cmake --build build --target ui_test > "$OUT/build.log" 2>&1; then
    echo "BUILD FAILED"; grep -E "error" "$OUT/build.log" | head -20; exit 1
  fi
fi
cd build || exit 1

failed_cases=0

run_case() {
  local name="$1"; shift
  local log="$OUT/$name.log"
  # alarm: a hung run (deadlock, waiting on the GPU) counts as TIMEOUT instead of blocking the suite.
  UI_TEST_SHOTS="$OUT" ASAN_OPTIONS=halt_on_error=1 perl -e 'alarm 240; exec @ARGV' ${BIN:-./ui_test} "$@" > "$log" 2>&1
  local code=$?
  local clean
  clean=$(sed 's/thread id: [0-9a-fx]* | //' "$log")
  local status="PASS"
  [ $code -ne 0 ] && status="FAIL(exit $code)"
  [ $code -eq 142 ] && status="TIMEOUT"
  local detail
  detail=$(echo "$clean" | grep -E "SUMMARY: AddressSanitizer|terminating due to|^\[repro\] survived|^\[fuzz\] done|^\[tree\] issues found|^\[leak\].*(OK|differ)|^\[drive\] summary" | sed 's/libc++abi: //' | tr '\n' ' ' | cut -c1-200)
  if echo "$clean" | grep -q "differ from baseline\|^\[tree\] issues found"; then
    [ "$status" = "PASS" ] && status="FAIL(leak/tree)"
  fi
  # invariant breaks count as failures even when the run happens not to crash.
  if echo "$clean" | grep -q "^\[listener\] stale\|^\[clip\] dead\|^\[draw-order\] broken"; then
    [ "$status" = "PASS" ] && status="FAIL(invariant)"
    detail="$detail $(echo "$clean" | grep -m1 -E '^\[draw-order\] node|^\[clip\] node|^\[listener\] (stale|.*holds)' | cut -c1-100)"
  fi
  if echo "$clean" | grep -q "^\[repro\] FAIL"; then
    [ "$status" = "PASS" ] && status="FAIL(repro)"
    detail="$detail $(echo "$clean" | grep -m1 '^\[repro\] FAIL' | cut -c1-120)"
  fi
  if echo "$clean" | grep -q "^\[drive\] FAIL"; then
    [ "$status" = "PASS" ] && status="FAIL(drive)"
    detail="$detail $(echo "$clean" | grep -m1 '^\[drive\] FAIL' | cut -c1-100)"
  fi
  # Vulkan validation errors are real bugs too (bad copies, out of range offsets, ...). Only messages
  # that cite a spec rule (VUID-...) count; the loader's "Searching for ICD / Loading layer" lines are info.
  local validation
  validation=$(echo "$clean" | grep -c "VUID-")
  if [ "$validation" -gt 0 ]; then
    [ "$status" = "PASS" ] && status="FAIL(validation x$validation)"
    detail="$detail $(echo "$clean" | grep -m1 -E 'Vulkan validation layer: vk' | cut -c1-120)"
  fi
  [ "${status#PASS}" = "$status" ] && failed_cases=$((failed_cases + 1))
  printf "%-22s %-28s %s\n" "$name" "$status" "$detail"
}

echo "== repros"
run_case repro-same-frame repro
run_case repro-clip repro-clip
run_case repro-image repro-image
run_case repro-scroll repro-scroll
run_case repro-touch repro-touch
run_case repro-reentry repro-reentry
run_case repro-api repro-api
# memory errors (use after free, dangling component refs) only show up reliably under AddressSanitizer.
# build-asan is configured with -fsanitize=address; it is rebuilt here when present.
if [ -d "$ROOT/build-asan" ]; then
  if [ -z "$UI_TEST_SKIP_BUILD" ]; then
    cmake --build "$ROOT/build-asan" --target ui_test > "$OUT/build-asan.log" 2>&1 || echo "ASAN BUILD FAILED"
  fi
  BIN="$ROOT/build-asan/ui_test" run_case asan-repro-reentry repro-reentry
fi
echo "== stress"
run_case stress stress
echo "== drive (scripted clicks / drags / wheel + screenshots)"
run_case drive drive
echo "== fuzz (seed scene ops/frame), all bug triggers enabled"
for args in "1 1 12" "2 2 12" "3 3 12" "4 1 1" "5 2 1" "6 3 1" "7 3 4" "8 2 4"; do
  set -- $args
  run_case "fuzz-s$1-sc$2-o$3" fuzz $1 $2 $3
done
echo "== $failed_cases failed, logs in $OUT"
exit $failed_cases
