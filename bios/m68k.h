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

#ifndef M68K_H
#define M68K_H

#include "portab.h"

/* Set the VBR on processors which support it, other do nothing. Must be called in supervisor. */
void m68k_set_vbr(void *vbr);

/* Safely disable the Memory Management Unit (but messes up LINE-F and ILLUEGAL INSTRUCTION vectors) */
void m68k_disable_mmu(void);

/* Returns TRUE if the given function was executed without BUS or ILLEGAL INSTRUCTION exceptions */
BOOL m68k_do_ignoring_exceptions(void (*)(void));

/* Send reset signal to peripherals */
static __inline__ void m68k_reset(void)
{
    __asm__ volatile (
        "reset"
        );
}

/* Send reset signal to peripherals */
static __inline__ void m68k_disable_caches(void)
{
    __asm__ volatile (
        ".dc.l 0x4e7b0002\t\n"        // move d0,cacr 68020-60 (even though bit usage differs!)
        "move.l  #0x00000808,d0\t\n"  // clear & disable instruction/data caches on
        :
        :
        : "d0", "cc"
        );
}

static __inline__ void m68k_set_stack(void *stack)
{
    __asm__ volatile (
        "move.l %0,sp"
        :
        : "g"(stack)
        : "cc"
    );
}

#endif
