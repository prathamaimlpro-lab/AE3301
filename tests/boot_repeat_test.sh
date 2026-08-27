#!/usr/bin/env bash
# Runs the Phase 0 kernel N times in QEMU and checks for the expected
# serial output each time. This is the automated check behind
# Milestone 0.1's exit criterion ("passes automated boot test 100/100
# runs") from the AE3301 roadmap. Intended to run in CI (see
# .github/workflows/ci.yml), not on the founder's local machine —
# there isn't one.
set -uo pipefail

RUNS="${1:-100}"
ISO="build/ae3301.iso"
EXPECTED="AE3301 Phase 0: boot OK"
FAIL=0

if [ ! -f "$ISO" ]; then
    echo "ISO not found at $ISO — run 'make iso' first." >&2
    exit 2
fi

for i in $(seq 1 "$RUNS"); do
    LOG="$(mktemp)"
    timeout 10s qemu-system-x86_64 \
        -cdrom "$ISO" \
        -serial "file:$LOG" \
        -display none \
        -no-reboot \
        -m 256M >/dev/null 2>&1 || true

    if grep -q "$EXPECTED" "$LOG"; then
        printf "run %3d/%d: OK\n" "$i" "$RUNS"
    else
        printf "run %3d/%d: FAIL (no expected output)\n" "$i" "$RUNS"
        FAIL=$((FAIL + 1))
    fi
    rm -f "$LOG"
done

echo "----"
echo "Passed: $((RUNS - FAIL))/$RUNS"

if [ "$FAIL" -ne 0 ]; then
    exit 1
fi
