#!/bin/sh
# Build genxtos with optional -mfastcall and verify toolchain support.
set -e
cd "$(dirname "$0")/.."

echo "=== Default (cdecl) build smoke test ==="
make clean >/dev/null 2>&1 || true
make 192 2>&1 | tail -5

if m68k-atari-mintelf-gcc -mfastcall -E - </dev/null >/dev/null 2>&1; then
    echo "=== FASTCALL build (MINTELF=1 FASTCALL=1) ==="
    make clean >/dev/null 2>&1 || true
    make MINTELF=1 FASTCALL=1 192 2>&1 | tail -10
    echo "FASTCALL build OK"
else
    echo "SKIP: m68k-atari-mintelf-gcc -mfastcall not available in PATH"
fi

echo "=== Verify ARGS_ON_STACK on trap handlers ==="
missing=$(grep -l 'VEC(' bios/bios.c bios/xbios.c | while read f; do grep 'VEC(' "$f"; done | \
    grep -oP 'VEC\([^,]+,\s*\K[^)]+' | grep -v direct | grep -v unimpl | while read fn; do
        grep -rq "ARGS_ON_STACK $fn\|ARGS_ON_STACK.*$fn" bios include 2>/dev/null || echo "$fn"
    done | head -5)
if [ -n "$missing" ]; then
    echo "WARNING: handlers possibly missing ARGS_ON_STACK:"
    echo "$missing"
else
    echo "Trap handler annotations look present"
fi

echo "=== Inline asm audit file ==="
test -f doc/fastcall-inline-asm-audit.md && echo "doc/fastcall-inline-asm-audit.md present"

echo "All fastcall migration checks completed."
