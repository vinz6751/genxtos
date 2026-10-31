#!/bin/sh
# Build GenXTOS inside distrobox st-gcc15 with gcc 15 (-mfastcall).
#
# The container overlay may expose an older m68k-atari-mintelf-gcc on PATH;
# prepend /usr/bin so gcc 15.2 is used.
#
# Usage:
#   tools/build-st-gcc15.sh [make goals...]
#
# Examples:
#   tools/build-st-gcc15.sh clean MINTELF=1 FASTCALL=1 256
#   tools/build-st-gcc15.sh MINTELF=1 FASTCALL=1 256
#
# After building a ROM target, etos256us.sym is regenerated when the map
# file is newer than the symbol file.

set -eu

ROOT="$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)"

# Avoid bash -lc "..." nesting: distrobox mangles nested quotes and $@.
# Pass make args as real argv; force a container PATH with host gcc 15 first.
distrobox enter st-gcc15 -- \
    env PATH="/usr/bin:/usr/m68k-atari-mintelf/bin:/usr/local/bin:/bin" \
    make -C "$ROOT" "$@"

if [ -f "$ROOT/emutos.map" ] && {
    [ ! -f "$ROOT/etos256us.sym" ] || [ "$ROOT/emutos.map" -nt "$ROOT/etos256us.sym" ]
}; then
    "$ROOT/tools/map2sym.sh" "$ROOT/emutos.map" > "$ROOT/etos256us.sym"
fi
