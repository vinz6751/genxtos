#!/bin/sh
# Run Hatari with debugger scripts that use CPU breakpoints correctly.
#
# Breakpoint scripts must use ":file" to run dump commands on hit, and the
# entry script must end with "c" only (no "quit") so --run-vbls can run.
#
# Usage:
#   tools/hatari-debug.sh [vbl_budget] [debugger.ini]
#
# Examples:
#   tools/hatari-debug.sh 600 tools/hatari-initinfo-debug.ini
#   tools/hatari-debug.sh 600 tools/hatari-debug-blank-only.ini

set -eu

HATARI="${HATARI:-/home/vincent/IT/Atari/Crossdev/hatari/build/src/hatari}"
ROOT="$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)"
TOS="${TOS:-$ROOT/etos256us.img}"
SYM="${SYM:-$ROOT/etos256us.sym}"
VBL_BUDGET=600
PARSE=tools/hatari-initinfo-debug.ini

if [ $# -gt 0 ] && case "$1" in ''|*[!0-9]*) false;; esac; then
    VBL_BUDGET=$1
    shift
fi

if [ $# -gt 0 ]; then
    PARSE=$1
    shift
fi

case "$PARSE" in
    /*) ;;
    *) PARSE="$ROOT/$PARSE" ;;
esac

if [ ! -f "$PARSE" ]; then
    echo "error: debugger script not found: $PARSE" >&2
    exit 1
fi

TIMEOUT_SEC=$((VBL_BUDGET / 50 + 20))

exec timeout -k 5 "$TIMEOUT_SEC" "$HATARI" \
    --tos "$TOS" \
    --natfeats true \
    --patch-tos false \
    --fast-boot false \
    --confirm-quit false \
    --run-vbls "$VBL_BUDGET" \
    --machine ste \
    --tos-res sthigh \
    --monitor mono \
    --parse "$PARSE" \
    "$@"
