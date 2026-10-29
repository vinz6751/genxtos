/*
 * deskstart.S - assembler startup code of the desktop
 *
 * Copyright (C) 1987 Digital Research Inc.
 *               2001 Lineo, Inc.
 *               2002-2024 The EmuTOS development team
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#include "asmdefs.h"

/* Desktop stack use can be estimated using CONF_DEBUG_DESK_STACK. */
#define DESKSTACKSIZE   2560    /* in bytes: must be multiple of 4 */

        .globl  _deskstart
        .globl  _desk_run_inprocess
#if CONF_DEBUG_DESK_STACK
        .globl  _deskstackbottom
#endif

        .extern _deskmain
        .extern _desk_boot_enter

        .text

_deskstart:

        movea.l a7,a5
        movea.l #deskustack,a7
        movea.l 4(a5),a5

        move.l  0xC(a5),d0
        add.l   0x14(a5),d0
        add.l   0x1C(a5),d0
        add.l   #0x100,d0
        move.l  d0,-(a7)
        move.l  a5,-(a7)
        clr.w   -(a7)
        move.w  #0x4A,-(a7)
        trap    #1                      // Mshrink
        lea     12(a7),a7

#if CONF_DEBUG_DESK_STACK
        move.l  #STACK_MARKER,d0        // fill stack with marker bytes
        lea     _deskstackbottom,a0
        lea     deskustack,a1
fill:
        move.l  d0,(a0)+
        cmpa.l  a0,a1
        jge     fill
#endif

restart:
        jsr     _deskmain               // Run the desktop
        tst.w   d0
        jeq     restart                 // handle read of .INF file

        clr.w   -(sp)
        trap    #1                      // Pterm

/*
 * Run deskmain on the desktop user stack in user mode.
 * Used when CONF_AES_INPROCESS skips Pexec/deskstart but still
 * needs a valid USP for AES trap #2 (menu_bar etc.).
 */
_desk_run_inprocess:
restart_ip:
        // USP only — do not touch a7/SSP while still in supervisor.
        // Setting SSP=deskustack made VDI trap #2 frames share the desk
        // user stack and smash everyobj/objc_draw return addresses
        // (especially on deep TEXTBLT paths). Same idea as gotopgm().
        movea.l #deskustack,a0
        move.l  a0,usp
        // Enter user mode but keep current IPL (same as gotopgm).
        // move.w #0,sr also forces IPL0 and has raced with DESK-MS/natfeat.
#ifdef __mcoldfire__
        move.w  sr,d0
        bclr    #13,d0
        move.w  d0,sr
#else
        andi.w  #0xdfff,sr              // user mode (SSP preserved)
#endif
        jsr     _desk_boot_enter
        jsr     _deskmain
        tst.w   d0
        jeq     restart_ip
        //
        // Super(0) from user: GEMDOS adopts USP as the new SSP and returns
        // the previous SSP in d0.  Our jsr return address still lives on that
        // old SSP (AES supervisor stack from sh_ldapp).  Switching a7 back
        // to it before rts is mandatory — addq/rts on the desk stack would
        // pop garbage (seen as PC=$2 → Illegal at $c85d0 when launching apps).
        //
        clr.l   -(sp)                   // Super(0): back to supervisor
        move.w  #0x20,-(sp)
        trap    #1                      // d0 = old SSP
        movea.l d0,sp                   // AES SSP (return addr intact)
        rts

.bss

// Space for the Stack:
_deskstackbottom:
        .ds.b   DESKSTACKSIZE
deskustack:
