/*
 * vdi_asm.S - Front end of the screen driver and mouse stuff
 *
 * Copyright 1999 Caldera, Inc. and Authors:
 * Copyright 2004-2024 The EmuTOS development team
 * Copyright      Steve Cavender
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#include "asmdefs.h"
#include "vdiext.h"

        .extern _local_pb
        .extern _CONTRL
        .extern _flip_y
        .extern _vdi_screen_driver
        .extern _vdishare               //see vdi_fill.c

#define lcl_ptsin _vdishare

/*
 * _vditrap - VDI trap dispatcher
 *
 * This functions checks for a trap-2 VDI call and jumps to the VDI dispatcher
 * then. It also provides a simple vq_gdos call and the "d0=-1" query function
 * that returns the dispatcher's address.
 *
 * Note that this function does not care about AES calls since the AES later
 * hooks into the trap-2 vector itself.
 */
        .globl  _vditrap
_vditrap:
        cmp.w   #0x73,d0                // VDI call?
        jne     no_vdi
        jsr     _GSX_ENTRY
        rte

no_vdi:
        cmp.w   #0xffff,d0              // query for jsr:able address?
        jeq     query

/*
 * Just like Atari TOS, we leave d0 unchanged, indicating the absence of
 * GDOS if this is called with -2 in the low-order word of d0.
 */
        rte

query:
        move.l  #_GSX_ENTRY,d0          // Return address of VDI dispatcher
        rte

/*
 * _GSX_ENTRY - front end of the GIOS for the screen driver
 */

#define W_1        2            // byte offset to first element of an array


        .text

        .globl  _GSX_ENTRY
_GSX_ENTRY:
#ifdef __mcoldfire__
        lea     -56(sp),sp
        movem.l d1-d7/a0-a6,(sp)        //Save the callers registers
#else
        movem.l d1-d7/a0-a6,-(sp)       //Save the callers registers
#endif

/* Make a local copy of the array pointers in the user's parameter block. */

        movea.l d1,a2           // get address of user's parameter block
        lea     _local_pb, a1    // get address of local storage area
        movea.l (a2)+,a0        // copy address of user's CONTRL aray
        move.l  a0,(a1)+        // save address of user's CONTRL array
        move.l  (a2)+,(a1)+     // save address of user's INTIN  array
        lea     lcl_ptsin,a3    // shared area used for local copy of PTSIN array
        move.l  a3,(a1)+        // save address of local PTSIN array
        move.l  (a2)+,a4        // get address of user's PTSIN array
        move.l  (a2)+,(a1)+     // save address of user's INTOUT array
        move.l  (a2),(a1)       // save address of user's PTSOUT array

/* Save a copy of the user's CONTRL[1] element on the stack, in case we change it. */
#ifdef __mcoldfire__
        moveq   #0,d0
#endif
        move.w  2(a0),d0        // get number of PTSIN entries from CONTRL[1]

/* Validate the number of entries in user's PTSIN array. */
        move.w  d0,-(sp)        // save number of PTSIN entries from CONTRL[1]
        jeq     no_ptsin        // for number of vertices = 0
size_ptsin:
        cmpi.w  #MAX_VERTICES,d0  // # of user vertices > # of allowable vertices?
        jle     ptsin_ok        // for user vertices =< # of allowable vertices
        move.w  #MAX_VERTICES,d0  // user vertices > # of allowable vertices
        move.w  d0,2(a0)        // put max # of vertices in user's CONTRL[1]
ptsin_ok:
        subq.l  #1,d0           // adjust coordinate count for loop
save_ptsin:
        move.l  (a4)+,(a3)+     // save x,y coordinate pair
#ifdef __mcoldfire__
        subq.l  #1,d0
        jpl     save_ptsin      // point to next x,y coordinate pair
#else
        dbra    d0,save_ptsin   // point to next x,y coordinate pair
#endif

no_ptsin:
/* Call screen which contains all the C routines for the SCREEN DRIVER. */
        jsr     _screen

/* Restore user's CONTRL[1] element from the stack. */
        movea.l _CONTRL,a0      // get address of user's CONTRL array
        move.w  (sp)+,2(a0)     // original # of PTSIN entries to CONTRL[1]

#ifdef __mcoldfire__
        movem.l (sp), d1-d7/a0-a6  // restore the caller's registers
        lea     56(sp), sp
#else
        movem.l (sp)+, d1-d7/a0-a6 // restore the caller's registers
#endif
        moveq   #0,d0
        move.w  _flip_y,d0      // set d0 to indicate magnitude/coordinate

        rts

#define F_SAVRDY        0               // save buffer status flag: 0:empty  1:full
#define F_SAVWID        1               // saved line width        0:word   1:longword


