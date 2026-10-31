#!/bin/sh
# Run Hatari with a guaranteed exit (no manual kill needed).
#
# Uses Hatari's --run-vbls as the primary shutdown mechanism, plus an outer
# timeout -k as a last resort if emulation wedges before the VBL budget.
#
# Usage:
#   tools/hatari-run.sh [vbl_budget] [-- extra hatari args ...]
#
# Examples:
#   tools/hatari-run.sh 500 -- --parse tools/hatari-freeze-debug.ini
#   tools/hatari-run.sh 3000 -- --parse tools/hatari-panic-debug.ini
#   tools/hatari-run.sh 800 -- --tos ./etos256us.img --natfeats true

set -eu

HATARI="${HATARI:-/home/vincent/IT/Atari/Crossdev/hatari/build/src/hatari}"
ROOT="$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)"
TOS="${TOS:-$ROOT/etos256us.img}"
VBL_BUDGET=500

if [ $# -gt 0 ] && case "$1" in ''|*[!0-9]*) false;; esac; then
    VBL_BUDGET=$1
    shift
fi

if [ $# -gt 0 ] && [ "$1" = "--" ]; then
    shift
fi

# Outer wall clock: VBL budget at 50 Hz + 15 s margin, hard-kill 5 s later.
TIMEOUT_SEC=$((VBL_BUDGET / 50 + 15))

exec timeout -k 5 "$TIMEOUT_SEC" "$HATARI" \
    --tos "$TOS" \
    --natfeats true \
    --patch-tos false \
    --fast-boot false \
    --confirm-quit false \
    --run-vbls "$VBL_BUDGET" \
    "$@"
