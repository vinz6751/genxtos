/*
 * lina_mouse_atari_.S - Asm routines for mouse support on Atari
 *
 * Copyright 1999 Caldera, Inc. and Authors:
 * Copyright 2004-2020 The EmuTOS development team
 * Copyright      Steve Cavender
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

        /* Imports */
        .extern _vbl_must_draw_mouse      // Non-zero means draw mouse form on vblank
        .extern _newx           // New mouse x position
        .extern _newy           // New mouse y position

        /* Exports */
        .globl  _mov_cur        // set's new x/y coordinates

        .text
/* Pass new cursor coordinates to the VBL handler in charge of drawing the mouse.
 * Inputs: d0.w: new X, d1.w: new Y
 * Outputs: None
 * Clobbered: None */
_mov_cur:
        tst.w   _HIDE_CNT       // is the mouse cursor hidden?
        bra.s    done           // yes - don't draw it now

        /* The cursor is not currently hidden.  Save the new coordinates for the
        * cursor and set a flag to signal the need to redraw it.  The cursor will
        * actually be drawn by a drawing routine added to the VBL queue.
        * This is a critical region so all interrupts must be turned off. */

        move.w  sr,-(sp)        // save current value of status register
        ori.w   #0x0700,sr      // mask off all interrupts

        move.w  d0,_newx        // save new cursor x-coordinate
        move.w  d1,_newy        // save new cursor y-coordinate

        bset.b  #0,_vbl_must_draw_mouse   // set the 'draw cursor' flag to signal to the VBL handler to redraw
        move.w  (sp)+,sr        // restore the value of the status register
done:
        rts
