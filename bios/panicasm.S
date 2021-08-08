/*
 * panicasm.S - support routines for panic debug messages
 *
 * Copyright (C) 2001-2021 The EmuTOS development team
 *
 * Authors:
 *  LVL     Laurent Vogel
 *  VB      Vincent Barrilliot
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#include "asmdefs.h"
#include "biosvecs.i"
#include "vectors.i"

        .text

        // TOS variables
        .extern _proc_dregs
        .extern _proc_usp
        .extern _proc_enum
        .extern _proc_stk
        .extern _proc_lives

        // C method
        .extern _dopanic

        // Exported functions
        .global _panic_init
        .global _panic
        .global _panic_exception


/* Initialise the panic handling */
_panic_init:
        clr.l   _proc_lives.w
        rts

/*
 * void panic(const char *fmt, ...);
 * save the registers, state and stack in 0x380, then
 * print the message on the console, and finally halt.
 * note: the stack dump will contain the attributes.
 * the exception number proc_enum will be zero.
 */
_panic:
        // supervisor mode, no interrupts
        move.w  #0x2700,sr
        // dump the registers including USP
        movem.l d0-d7/a0-a7,_proc_dregs.w
        move.l  usp,a0
        move.l  a0,_proc_usp.w

        // tell dopanic() that there are arguments following
        clr.l   _proc_enum.w
        // dump 16 words from the stack
        movem.l (sp),d0-d7
        movem.l d0-d7,_proc_stk.w
        // magic value
        move.l  #0x12345678,_proc_lives.w
        // at this time, we have not altered the stack, so we can jump to the
        // C routine directly in kprint.c
        jmp     _dopanic

/*
 * TSimilar to the above, but to be called by 68000 exception vectors
 */
_panic_exception:
        /* TODO: this is unfortunately a copy/paste of the above */
        move.w  #0x2700, sr
        // dump the registers, including USP
        movem.l d0-d7/a0-a6,_proc_dregs.w
        move.l  usp,a0
        move.l  a0,_proc_usp.w

        // compute and dump the exception number
        move.l  (sp)+,d0
        // dump the correct value for sp
        move.l  sp,(_proc_aregs+28).w
        lea     _def_vecs_table, a0
        sub.l   a0, d0
        lsr.l   #1, d0
        add.l   #1, d0
        move.l  d0, _proc_enum.w

#if CONF_WITH_ADVANCED_CPU
        // D0 is 1 if the exception number could not be computed
        subq    #1, d0
        bne     dump_stack
        // exception vector offset is present on stack for 68010 and above
        tst.w   _longframe
        beq     dump_stack
        move.w  6(sp), d0
        ext.l   d0
        andi.l  #0xfff, d0
        lsr.l   #2, d0
        move.l  d0, _proc_enum.w
#endif /* CONF_WITH_ADVANCED_CPU */

dump_stack:
        // dump 16 words from the stack
        movem.l (sp),d0-d7
        movem.l d0-d7,_proc_stk.w
        // magic value
        move.l  #0x12345678, _proc_lives.w
        // restore a sane stack
        // this stack must be large enough for dopanic(); as of
        // may 2015, this is about 460 bytes
        lea     panic_stack,sp
        jmp     _dopanic


        .bss
        .even

        .ds.w    256            // this stack is only used by dopanic()
panic_stack:
