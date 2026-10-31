#!/bin/sh
# Time VDI text, solid fills, and lines in Hatari and keep before/after logs.
#
# Each case repeats the primitive for one run of at least two seconds.
# Speed is draws/ticks.  One line per case:
#
#   text_8x16_left_w16 rez=2 draws=1280 ticks=400 samples=1 sample=1280 lo=400 hi=400
#
# Usage:
#   tools/rastbench.sh emutos              # TOS=/path/to/etos256fr-1.4.img
#   tools/rastbench.sh before [high|low|medium...]
#   tools/rastbench.sh after medium
#
# TOS selects the ROM (default: etos256fr.img, else etos256us.img).
# With no modes, high and low are timed. medium is 640x200.
# Logs land in tests/rastbench/rastbench-<label>-<mode>.txt.
# Ratios are this label's draws/ticks divided by each other label on disk.

set -eu

ROOT="$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)"
LABEL="${1:-}"
shift || true
if [ "$LABEL" != "emutos" ] && [ "$LABEL" != "before" ] && [ "$LABEL" != "after" ]; then
    echo "usage: $0 emutos|before|after [high|low|medium...]" >&2
    exit 1
fi
if [ "$#" -eq 0 ]; then
    set -- high low
fi

if [ -f "$ROOT/etos256fr.img" ]; then
    TOS="${TOS:-$ROOT/etos256fr.img}"
elif [ -f "$ROOT/etos256us.img" ]; then
    TOS="${TOS:-$ROOT/etos256us.img}"
else
    echo "error: no etos256fr.img or etos256us.img (set TOS=)" >&2
    exit 1
fi

HATARI="${HATARI:-/home/vincent/IT/Atari/Crossdev/hatari/build/src/hatari}"
SRC="$ROOT/tests/rastbench/rastbench.c"
PRG="$ROOT/tests/rastbench/RASTBENCH.PRG"
GEMDOS="$ROOT/tests/rastbench/gemdos"
VBL_BUDGET="${RASTBENCH_VBLS:-50000}"

if [ ! -x "$HATARI" ]; then
    echo "error: hatari not found: $HATARI" >&2
    exit 1
fi

echo "Building $PRG ..."
distrobox enter st-gcc15 -- bash -lc \
    "export PATH=/usr/bin:/usr/m68k-atari-mintelf/bin:\$PATH && \
     m68k-atari-mintelf-gcc -m68000 -ffreestanding -nostdlib -nostartfiles \
       -fno-exceptions -fno-asynchronous-unwind-tables \
       -fomit-frame-pointer -O2 -Wall -Wextra \
       -Wl,-e,_start -Wl,-s \
       -o '$PRG' '$SRC' -lgcc"

mkdir -p "$GEMDOS"
cp "$PRG" "$GEMDOS/RASTBENCH.PRG"
cat > "$GEMDOS/EMUDESK.INF" <<'EOF'
#R 01
#Z 01 C:\RASTBENCH.PRG@
#E 1A E1 FF 02 00
#Q 41 40 43 40 43 40
#M 00 00 01 FF A DISK A@ @
#M 02 00 00 FF C DISK C@ @
#T 00 08 03 FF   TRASH@ @
#F 06 07 C:\RASTBENCH.PRG@ *.@ 000 @
EOF

run_one() {
    mode=$1
    shift
    log="$ROOT/tests/rastbench/rastbench-${LABEL}-${mode}.log"
    out="$ROOT/tests/rastbench/rastbench-${LABEL}-${mode}.txt"
    rm -f "$GEMDOS/RASTBENCH.TXT" "$out"

    echo "Running $mode ($LABEL) TOS=$TOS ..."
    set +e
    TOS="$TOS" "$ROOT/tools/hatari-run.sh" "$VBL_BUDGET" -- \
        --machine ste \
        "$@" \
        --harddrive "$GEMDOS" \
        --fast-forward on \
        --sound off \
        >"$log" 2>&1
    set -e

    if [ ! -f "$GEMDOS/RASTBENCH.TXT" ]; then
        echo "error: $mode produced no RASTBENCH.TXT (see $log)" >&2
        exit 1
    fi
    if ! grep -q 'RASTBENCH done' "$GEMDOS/RASTBENCH.TXT"; then
        echo "error: $mode did not finish (see $log)" >&2
        exit 1
    fi
    if ! grep -q '^text_6x6_left_w16 ' "$GEMDOS/RASTBENCH.TXT"; then
        echo "error: $mode has no timings (see $log)" >&2
        exit 1
    fi
    if awk '$2 ~ /^rez=/ { for (i = 1; i <= NF; i++) { split($i, kv, "="); if (kv[1] == "lo" && kv[2]+0 < 400) short++ } } END { exit (short ? 0 : 1) }' "$GEMDOS/RASTBENCH.TXT"; then
        echo "error: $mode has a sample shorter than 2s (see $log)" >&2
        exit 1
    fi
    cp "$GEMDOS/RASTBENCH.TXT" "$out"
    echo "Wrote $out"
    grep -E '^(text_|fill_|vline_|hline_|dline_|blitmode=|font |v_)' "$out" || true
}

for mode in "$@"; do
    case "$mode" in
        high)   run_one high   --tos-res sthigh --monitor mono ;;
        low)    run_one low    --tos-res stlow  --monitor rgb ;;
        medium) run_one medium --tos-res stmed  --monitor rgb ;;
        *)
            echo "error: unknown mode $mode" >&2
            exit 1
            ;;
    esac
done

echo
echo "=== $LABEL ==="
for mode in "$@"; do
    a="$ROOT/tests/rastbench/rastbench-${LABEL}-${mode}.txt"
    echo "-- $mode --"
    any=0
    for other in emutos before after; do
        b="$ROOT/tests/rastbench/rastbench-${other}-${mode}.txt"
        if [ "$other" = "$LABEL" ] || [ ! -f "$b" ]; then
            continue
        fi
        any=1
        echo "vs $other"
        awk -v label="$LABEL" -v other="$other" '
        function field(name,    i, kv) {
            for (i = 1; i <= NF; i++) {
                split($i, kv, "=")
                if (kv[1] == name)
                    return kv[2] + 0
            }
            return 0
        }
        FNR == NR && $2 ~ /^rez=/ {
            split($2, r, "=")
            key = $1 " " r[2]
            draws[key] = field("draws")
            ticks[key] = field("ticks")
            next
        }
        $2 ~ /^rez=/ {
            split($2, r, "=")
            key = $1 " " r[2]
            nowd = field("draws")
            nowt = field("ticks")
            if (draws[key] > 0 && ticks[key] > 0 && nowt > 0)
                printf "%-36s rez=%s  ratio=%.3f  %s_ticks=%d..%d\n",
                    $1, r[2], (nowd / nowt) / (draws[key] / ticks[key]),
                    label, field("lo"), field("hi")
            else
                printf "%-36s rez=%s  draws=%d ticks=%d lo=%d hi=%d\n",
                    $1, r[2], nowd, nowt, field("lo"), field("hi")
        }
    ' "$b" "$a"
        done
        if [ "$any" -eq 0 ]; then
            grep -E ' rez=' "$a" || true
            echo "(no other log yet)"
        fi
    done
