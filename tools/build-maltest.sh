#!/bin/sh
# Build freestanding MALTEST.PRG (no mint CRT) for Hatari AUTO-folder tests.
#
# Usage:
#   tools/build-maltest.sh [install_dir]
#
# With no argument, installs to tools/hatari-showmem/gemdos/AUTO/ and
# $GEMDOS_C/AUTO/ when that tree exists.

set -eu

ROOT="$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)"
TOOL="$ROOT/tools"
CC="${CC:-m68k-atari-mintelf-gcc}"
OBJCOPY="${OBJCOPY:-m68k-atari-mintelf-objcopy}"
PRG="$TOOL/MALTEST.TTP"
INSTALL_ROOT="${1:-}"

if ! command -v "$CC" >/dev/null 2>&1; then
    echo "error: $CC not found (try: distrobox enter st-gcc15 -- $0)" >&2
    exit 1
fi

echo "Building freestanding MALTEST with $CC ..."
# -ffreestanding -nostdlib -nostartfiles: no mint CRT (~50KB+)
# -lgcc: soft div/mod for u32dec / show_bytes only
"$CC" -m68000 -ffreestanding -nostdlib -nostartfiles \
    -fno-exceptions -fno-asynchronous-unwind-tables \
    -fomit-frame-pointer \
    -O2 -Os -Wall -Wextra \
    -Wl,-e,_start -Wl,-s \
    -o "$PRG" "$TOOL/maltest.c" -lgcc

# mintelf-strip often rejects freestanding PRG PHDRs; drop comment via objcopy.
if command -v "$OBJCOPY" >/dev/null 2>&1; then
    "$OBJCOPY" -R .comment "$PRG" "$PRG.tmp" 2>/dev/null && mv "$PRG.tmp" "$PRG" || rm -f "$PRG.tmp"
fi

SIZE=$(wc -c < "$PRG" | tr -d ' ')
echo "Built $PRG ($SIZE bytes)"
if [ "$SIZE" -gt 10240 ]; then
    echo "warning: MALTEST is larger than 10KB ($SIZE); expected freestanding few-KB binary" >&2
fi

install_one() {
    dest_dir=$1
    mkdir -p "$dest_dir"
    cp "$PRG" "$dest_dir/MALTEST.PRG"
    echo "Installed $dest_dir/MALTEST.PRG"
}

if [ -n "$INSTALL_ROOT" ]; then
    install_one "$INSTALL_ROOT/AUTO"
    exit 0
fi

install_one "$TOOL/hatari-showmem/gemdos/AUTO"
install_one "$TOOL/hatari-showmem/floppy/AUTO"

GEMDOS_C="${GEMDOS_C:-/home/vincent/IT/Atari/Disques/c}"
if [ -d "$GEMDOS_C" ]; then
    install_one "$GEMDOS_C/AUTO"
fi

echo "Done."
