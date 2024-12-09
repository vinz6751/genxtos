/*
 * aciavecs.S - exception handling for ikbd/midi acias.
 *
 * Copyright (C) 2001-2022 The EmuTOS development team
 *
 * Authors:
 *  LVL  Laurent Vogel
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

// the following text is taken from the Atari Compendium, xbios(0x22)
//
// Kbdvbase() returns a pointer to a system structure KBDVECS which
// is defined as follows:
//
// typedef struct
// {
//   void (*midivec)( UBYTE data_d0 );     /* Handle incoming MIDI byte in d0 */
//   void (*vkbderr)( UBYTE data_d0 );     /* IKBD error or overrun */
//   void (*vmiderr)( UBYTE data_d0 );     /* MIDI error or overrun */
//   void (*statvec)( UBYTE *packet_a0 );  /* IKBD Status */
//   void (*mousevec)( UBYTE *packet_a0 ); /* IKBD Mouse */
//   void (*clockvec)( UBYTE *packet_a0 ); /* IKBD Clock */
//   void (*joyvec)( UBYTE*packet_a0);     /* IKBD Joystick */
//   void (*midisys)( void );              /* Detect if MIDI interrupt and if so, call midivec/vmiderr */
//   void (*ikbdsys)( void );              /* Detect if IKBD interrupt and if so, call kbdvec/mousevec/joyvec/vkderr */
// } KBDVECS;
//
// In addition to the documented features, it was deemed necessary to add
// the following undocumented features, located in memory immediately before
// and after the documented part of the KBDVECS structure:
//
// struct UNDOCUMENTED {
//   void (*kbdvec)( UBYTE data_d0, IOREC *iorec_a0 );   /* KBD Input, TOS >= 2.0 */
//   KBDVECS kbdvecs;
//   UBYTE ikbdstate;
//   UBYTE kbdlength;
// };
//
//- kbdvec (undocumented feature of TOS >= 2.0) is called with the
//  received data byte in d0, and a pointer to the ikbd iorec in a0.
//- The ikbdstate description in the Compendium is wrong. It should read:
//  "When the ikbdstate variable is non-zero, it means that a packet is
//  currently being retrieved. In that case, kbdlength represents the number
//  of remaining bytes to retrieve that are part of an IKBD packet."

/* Known differences between this and Atari TOS:
 *
 * Buffer usage: both EmuTOS and Atari TOS use a separate buffer for
 * joystick reports.  However EmuTOS uses kbdbuf to accumulate all other
 * IKBD packets, whereas Atari TOS uses four different buffers:
 *  . kbdbuf (for response 0xf6)
 *  . mousebuf (for response 0xf7)
 *  . relmousebuf (for response 0xf8-0xfb)
 *  . clockbuf (for response 0xfc)
 */

#include "asmdefs.h"

        // Exports
        .globl _kbdlength
#if CONF_WITH_IKBD_ACIA || CONF_WITH_MIDI_ACIA
        .globl  _int_acia
#endif

#if CONF_WITH_EXTENDED_MOUSE
        .globl  _mousexvec
#endif
        .globl  _kbdvec
        .globl  _kbdvecs
        .globl  _call_mousevec
        .globl _call_kbdint

        .bss

// ==== KBDVBASE =============================================================
// This is now the table of routines for managing midi and keyboard data
// in packets from IKBD (shown by a0 and 4(sp))

#if CONF_WITH_EXTENDED_MOUSE
_mousexvec:     .ds.l   1       // mouse-routine for additional buttons
#endif
_kbdvec:        .ds.l   1       // must be right before _kbdvecs
_kbdvecs: // This is what is returned by Kbdvbase and is a KBDVECS structure
midivec:        .ds.l   1       // MIDI input
vkbderr:        .ds.l   1       // keyboard error
vmiderr:        .ds.l   1       // MIDI-Error
statvec:        .ds.l   1       // IKBD-Status
mousevec:       .ds.l   1       // mouse-routine, receives pointer to mouse packet in a0
clockvec:       .ds.l   1       // time-routine
joyvec:         .ds.l   1       // joystick-routinee
midisys:        .ds.l   1       // MIDI-systemvector
ikbdsys:        .ds.l   1       // IKBD-systemvector
ikbdstate:      .ds.b   1       // action to take upon packet completion
_kbdlength:     .ds.b   1       // number of bytes remaining in current packet
        .even

        .text

#if CONF_WITH_IKBD_ACIA || CONF_WITH_MIDI_ACIA

// ==== Int 0x118 - midi/kbd interrupt routine ================
//

_int_acia:
        // save scratch regs
        /* LVL: Strictly speaking there is no specification telling what
         * registers can be freely used by the vectors of the kbdvecs struct.
         * Theoretically d0-d1/a0-a1 would be sufficient here.
         */
#ifdef __mcoldfire__
        lea     -32(sp),sp
        movem.l d0-d3/a0-a3,(sp)
#else
        movem.l d0-d3/a0-a3,-(sp)
#endif

        .globl _int_acia_c
        jbsr    _int_acia_c // Call the C implementation

        // restore scratch regs
#ifdef __mcoldfire__
        movem.l (sp),d0-d3/a0-a3
        lea     32(sp),sp
#else
        movem.l (sp)+,d0-d3/a0-a3
#endif
        rte

#endif /* CONF_WITH_IKBD_ACIA || CONF_WITH_MIDI_ACIA */


        .text

_call_kbdint:
        move.w  d0,-(sp)
        // call the C routine in ikbd.c to do the work.
        jsr     _kbd_int
        addq.l  #2,sp
        rts


/******************************************************************************/
/* Call mousevec from C                                                       */
/******************************************************************************/

_call_mousevec:
        move.l  4(sp),a0
        move.l  mousevec,a1
#ifdef __mcoldfire__
        lea     -44(sp),sp
        movem.l d2-d7/a2-a6,(sp)
#else
        movem.l d2-d7/a2-a6,-(sp)
#endif
        jsr     (a1)
#ifdef __mcoldfire__
        movem.l (sp),d2-d7/a2-a6
        lea     44(sp),sp
#else
        movem.l (sp)+,d2-d7/a2-a6
#endif
        rts

# if 0

#if CONF_WITH_FLEXCAN || CONF_SERIAL_IKBD || defined(MACHINE_LISA)

/******************************************************************************/
/* Call ikbdraw from C                                                        */
/******************************************************************************/

        .globl  _call_ikbdraw
_call_ikbdraw:
        moveq   #0,d0
        move.b  5(sp),d0                // raw IKBD byte
        jra     _ikbdraw

#endif /* CONF_WITH_FLEXCAN || CONF_SERIAL_IKBD || defined(MACHINE_LISA) */


        .globl _callKbdvecs_wrapper
_callKbdvecs_wrapper:
#ifdef __mcoldfire__
        lea     -44(sp),sp
        movem.l d2-d7/a2-a6,(sp)
#else
        movem.l d2-d7/a2-a6,-(sp)
#endif
        .globl _callKbdvecs
        jsr     _callKbdvecs
#ifdef __mcoldfire__
        movem.l (sp),d2-d7/a2-a6
        lea     44(sp),sp
#else
        movem.l (sp)+,d2-d7/a2-a6
#endif
        rts
#endif

#ifdef MACHINE_AMIGA

/******************************************************************************/
/* Call joyvec from C                                                         */
/******************************************************************************/

        .globl  _call_joyvec
_call_joyvec:
        move.l  4(sp),a0
        move.l  joyvec,a1
#ifdef __mcoldfire__
        lea     -44(sp),sp
        movem.l d2-d7/a2-a6,(sp)
#else
        movem.l d2-d7/a2-a6,-(sp)
#endif
        jsr     (a1)
#ifdef __mcoldfire__
        movem.l (sp),d2-d7/a2-a6
        lea     44(sp),sp
#else
        movem.l (sp)+,d2-d7/a2-a6
#endif
        rts

#endif /* MACHINE_AMIGA */
