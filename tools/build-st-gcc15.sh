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

exec distrobox enter st-gcc15 -- bash -lc \
    "export PATH=/usr/bin:/usr/m68k-atari-mintelf/bin:\$PATH && cd '$ROOT' && make \"\$@\" && \
     if [ -f emutos.map ] && { [ ! -f etos256us.sym ] || [ emutos.map -nt etos256us.sym ]; }; then \
         ./tools/map2sym.sh emutos.map > etos256us.sym; \
     fi" \
    -- "$@"

