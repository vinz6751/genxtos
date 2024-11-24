/*
 * cartridge_atari.S - Atari cartridge port supporting functions
 *
 * Copyright (C) 2001-2024 The EmuTOS development team
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#include "config.h"

#if CONF_WITH_CARTRIDGE

    .equ    CARTRIDGE_BASE,             0x00fa0000 // Base of ROM port cartridge
    .equ    CARTRIDGE_DIAGNOSTIC_MAGIC, 0xfa52235f // diagnostic cartridge magic number


    .globl _cartridge_atari_boot
_cartridge_atari_boot:
        lea .rts(pc),a6 // return address
        jmp CARTRIDGE_BASE+4
.rts:   rts



/*
 * void run_cartridge_applications(WORD typebit);
 *
 * Run all cartridge applications of the specified type.
 *
 * typebit: application type bit number which must be set to 1 to match
 */
    .globl  _run_cartridge_applications
_run_cartridge_applications:
        move.w  4(sp),d0                // application bit number
        lea     CARTRIDGE_BASE, a0
#ifdef __mcoldfire__
        move.l  #0xABCDEF42, d1
        cmp.l   (a0)+, d1               // is cartridge present?
#else
        cmp.l   #0xABCDEF42, (a0)+      // is cartridge present?
#endif
        jne     cartover                // no -> cartover
testtype:
        btst    d0, 4(a0)               // What type?
        jeq     nextapp

#ifdef __mcoldfire__
        lea     -60(sp), sp
        movem.l d0-d7/a0-a6, (sp)       // save registers
#else
        movem.l d0-d7/a0-a6, -(sp)      // save registers
#endif
        move.l  4(a0),d0                // d0 = address of cartridge init
        andi.l  #0x00ffffff,d0          // make it 24-bit clean
        movea.l d0,a0
        jsr     (a0)                    // execute app in cartridge
#ifdef __mcoldfire__
        movem.l (sp), d0-d7/a0-a6       // restore registers
        lea     60(sp), sp
#else
        movem.l (sp)+, d0-d7/a0-a6      // restore registers
#endif
nextapp:
        tst.l   (a0)                    // another application?
        move.l  (a0), a0
        jne     testtype
cartover:
        rts
#endif /* CONF_WITH_CARTRIDGE */
