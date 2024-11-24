/*
 * m68k.S - Motorola 68000 specific commands
 *
 * Copyright (C) 2024 by Authors:
 *
 * Authors:
 *  VB   Vincent Barrilliot
 * With credits to EmuTOS authors:
 *  MAD  Martin Doering
 *  PES  Petr Stehlik
 *  jfdn Jean-François DEL NERO
 *  VRI  Vincent Rivière
 *  RFB  Roger Burrows
 *
 * This file is not part of the original EmuTOS distribution.
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#include "config.h"
#include "asmdefs.h"

#define VEC_BUS_ERROR 0x08
#define VEC_ILLEGAL   0x10
#define VEC_LINE_F    0x2C


    .globl _m68k_set_vbr
_m68k_set_vbr:
    // Note: LisaEm doesn't properly implement movec/pmove illegal instructions, so we just avoid them on MACHINE_LISA.
#if !defined(__mcoldfire__) && !defined(MACHINE_LISA)
    move.l  4(sp),d0
    move.l sp,a0
    move.l  VEC_ILLEGAL,d1     // Save VEC_ILLEGAL illegal instruction vector
    move.l  #.end,VEC_ILLEGAL
    MOVEC_D0_VBR               // Set Vector Base Register
.end:
    move.l  d1,VEC_ILLEGAL     // Restore
    move.l  a0,sp
#endif
    rts


    .globl _m68k_reset
_m68k_reset:
    reset
    rts


    .globl _m68k_do_ignoring_exceptions
_m68k_do_ignoring_exceptions:
    move.l sp,a3
    move.l VEC_BUS_ERROR,d3
    move.l VEC_ILLEGAL,d4
    move.l #.bus_error,VEC_BUS_ERROR
    move.l #.bus_error,VEC_ILLEGAL
    move.l 4(sp),a0
    jsr    (a0)
    moveq  #1,d0 // Function was executed without bus or illegal instruction error: TRUE
    bra.s  .restore
.bus_error:
    clr.w  d0 // Error detected: FALSE
.restore:
    move.l d3,VEC_BUS_ERROR
    move.l d4,VEC_ILLEGAL
    move.l a3,sp
    rts


// Disable the Memory Management Unit
// This messes up the VEC_LINE_F and possibly VEC_ILLEGAL vectors !
    .globl _m68k_disable_mmu
_m68k_disable_mmu:
#if !defined(__mcoldfire__) && !defined(MACHINE_LISA)
        move.l  sp,a3
        // we're going to PMOVE, which only the 68030 has, it's a line F exception on other procs.
        move.l  #.not_68030,VEC_LINE_F
        lea     .zero(pc),a0
        PMOVE_A0_TTR0                  // initialise the TTRs (ACRs on a 68ec030)
        PMOVE_A0_TTR1                  // if we got here, we must be some kind of 68030
        move.l  #.done,VEC_LINE_F      // since PMOVE (A0),TC doesn't exist on a 68ec030,
        PMOVE_A0_TC                    // we're done if we get a lineF exception ...
        bra.S   .done
.not_68030: // could be anything but a 68030
        move.l  #.done,VEC_ILLEGAL     // if a MOVEC causes an exception (000-020), we're done
        moveq   #0,d0
        MOVEC_D0_ITT0                  // first we initialise the TTRs (ACRs on a 68ec040)
        MOVEC_D0_ITT1
        MOVEC_D0_DTT0
        MOVEC_D0_DTT1
        MOVEC_D0_TC                    // disable translation on 68040-60 (will error ou on a 68ec040 or 68ex060)
.done:  move.l a3,sp
        rts
.zero:  .dc.l 0
#endif