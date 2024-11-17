/*
 * timer_S.c - OS-level assembly functions related to timers
 *
 * Copyright (C) 2001 Martin Doering
 * Copyright (C) 2001-2024 The EmuTOS development team
 *
 * Authors:
 *  LVL   Laurent Vogel
 *  MAD   Martin Doering
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */


#include "asmdefs.h"

        .globl  _int_timerc

#define REAL_TIMER_C_HANDLER (CONF_WITH_MFP && !CONF_COLDFIRE_TIMER_C)


// ==== Timer C interrupt handler ============================================
_int_timerc:
        addq.l  #1, _hz_200.w           // increment 200 Hz counter
#ifdef __mcoldfire__
        // Save early ColdFire registers
        move.l  d0,-(sp)
        move.l  a0,-(sp)
#elif defined(MACHINE_A2560U) || defined(MACHINE_A2560X)
        move.w  #0x0400,IRQ_PENDING_GRP1  // adjust this for HZ200_TIMER_NUMBER to acknowledge the correct GAVIN Timer interrupt
#endif

#ifdef __mcoldfire__
        lea     _timer_c_sieve,a0
        move.w  (a0),d0
        swap    d0
        move.w  (a0),d0
        lsl.l   #1,d0                   // check for 4th call
        swap    d0
        move.w  d0,(a0)
        jpl     timerc_end
#else
        rol.w   _timer_c_sieve          // check for 4th call
        jpl     timerc_end
#endif

#ifdef __mcoldfire__
        // On ColdFire, d0 and a0 were saved earlier
        lea     -52(sp),sp
        movem.l d1-d7/a1-a6,(sp)        // save registers
#else
        movem.l d0-d7/a0-a6,-(sp)       // save registers
#endif

        // repeat keys
        jsr     _kb_timerc_int

#if CONF_WITH_YM2149
        // dosound support
        jsr     _sndirq
#endif

        move.w  _timer_ms.w, -(sp)
        move.l  _etv_timer.w, a0
        jsr     (a0)                    // jump to etv_timer routine
        addq.l  #2, sp                  // correct stack

#ifdef __mcoldfire__
        // On ColdFire, d0 and a0 will be restored later
        movem.l (sp),d1-d7/a1-a6
        lea     52(sp),sp
#else
        movem.l (sp)+,d0-d7/a0-a6
#endif

timerc_end:
#if REAL_TIMER_C_HANDLER
#ifdef __mcoldfire__
        lea     0xfffffa11.w, a0
        move.b  #0xdf,(a0)              // clear interrupt service bit
#else
        move.b  #0xdf,0xfffffa11.w      // clear interrupt service bit
#endif
#endif /* REAL_TIMER_C_HANDLER */

#ifdef __mcoldfire__
        // Restore late ColdFire registers
        move.l  (sp)+, a0
        move.l  (sp)+, d0
#endif

        // Note for non-Atari hardware:
        // It is important to return with RTE here, because FreeMiNT may have
        // hooked this routine. Every fourth interrupt, it pushes an additional
        // exception stack frame pointing to a pseudo-VBL routine. In that case,
        // the RTE below jumps to that pseudo-VBL. Then that one will return to
        // our caller with RTE.
        rte