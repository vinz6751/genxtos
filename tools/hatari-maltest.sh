#!/bin/sh
# Run in-ROM MALTEST-MS smoke test (NATFEAT_DEBUG) before AUTO/AES.
#
# AUTO folder Pexec is currently broken under FASTCALL (Fsfirst OSM loop),
# so the ROM itself prints Mxalloc/Malloc results to NF_STDERR.
#
# Usage:
#   TOS=$PWD/etos256us.img tools/hatari-maltest.sh [vbl_budget]

set -eu

ROOT="$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)"
TOS="${TOS:-$ROOT/etos256us.img}"
LOG="${MALTEST_LOG:-$ROOT/hatari-maltest.log}"

if [ ! -f "$TOS" ]; then
    echo "error: TOS ROM not found: $TOS" >&2
    exit 1
fi

VBL_BUDGET=2000
if [ $# -gt 0 ] && case "$1" in ''|*[!0-9]*) false;; esac; then
    VBL_BUDGET=$1
    shift
fi
if [ $# -gt 0 ] && [ "$1" = "--" ]; then
    shift
fi

echo "Capturing output to $LOG" >&2
echo "Looking for MALTEST-MS lines (in-ROM heap smoke test)..." >&2

# No floppy: avoid AUTO Fsfirst OSM hang. Use default Hatari GEMDOS drives.
"$ROOT/tools/hatari-run.sh" "$VBL_BUDGET" -- \
    --tos "$TOS" \
    --machine ste \
    --tos-res sthigh \
    --monitor mono \
    "$@" 2>&1 | tee "$LOG"

echo "--- MALTEST-MS ---" >&2
if grep -E 'MALTEST-MS' "$LOG"; then
    :
else
    echo "warning: no MALTEST-MS lines (rebuild with NATFEAT_DEBUG=1?)" >&2
fi
echo "------------------" >&2
