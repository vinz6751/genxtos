#!/bin/sh
# Compile-test fastcall migration artifacts (no full ROM link required).
set -e
cd "$(dirname "$0")/.."
CC=${CC:-m68k-atari-mint-gcc}
CFLAGS="-m68000 -mshort -O2 -Iinclude -fomit-frame-pointer"

echo "Testing cdecl asm objects..."
$CC $CFLAGS -c \
    util/memset.S util/memmove.S util/setjmp.S util/miscasm.S \
    util/stringasm.S util/optimopt.S \
    bdos/rwa.S bios/aciavecs.S aes/gemdosif.S vdi/linea_mouse_.S

echo "Testing FASTCALL_BUILD asm objects..."
$CC $CFLAGS -DFASTCALL_BUILD -c \
    util/memset.S util/memmove.S util/setjmp.S util/miscasm.S \
    util/stringasm.S util/optimopt.S \
    bdos/rwa.S bios/aciavecs.S aes/gemdosif.S vdi/linea_mouse_.S

echo "Testing trap handler C with ARGS_ON_STACK..."
$CC $CFLAGS -c bdos/bdosmain.c -o /tmp/bdosmain.o
$CC $CFLAGS -c bios/screen.c -o /tmp/screen.o

if command -v m68k-atari-mintelf-gcc >/dev/null 2>&1 && \
   m68k-atari-mintelf-gcc -mfastcall -E - </dev/null >/dev/null 2>&1; then
    echo "Testing MINTELF FASTCALL=1 compile of bdosmain.c..."
    m68k-atari-mintelf-gcc $CFLAGS -mfastcall -fleading-underscore \
        -Wa,--register-prefix-optional -DELF_TOOLCHAIN \
        -c bdos/bdosmain.c -o /tmp/bdosmain_fc.o
    echo "MINTELF fastcall C compile OK"
else
    echo "SKIP: m68k-atari-mintelf-gcc -mfastcall not available"
fi

echo "Fastcall compile tests passed."
