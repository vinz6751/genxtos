/*
 * linea_mouse_.S - line-A mouse assembly routines
 *
 * Copyright 1999 Caldera, Inc. and Authors:
 * Copyright 2004-2020 The EmuTOS development team
 * Copyright      Steve Cavender
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#include "config.h"

        /* Imports */
        .extern _GCURX          // graphics cursor x-coordinate
        .extern _GCURY          // graphics cursor y-coordinate
        .extern _HIDE_CNT       // graphics cursor hide count
        .extern _MOUSE_BT       // mouse button state
        .extern _cur_ms_stat    // current mouse status
        .extern _user_but       // user button vector
        .extern _user_mot       // user motion vector
        .extern _user_cur       // user cursor vector
        .extern _DEV_TAB        // address of device table
#if CONF_WITH_EXTENDED_MOUSE
        .extern _user_wheel     // user wheel vector
        .extern _old_statvec    // previous IKBD status packet vector
#endif
        /* Exports */
        .globl  _mousevec_handler      // mouse interrupt routine
#if CONF_WITH_EXTENDED_MOUSE
        .globl  _wheel_int      // wheel interrupt routine
        .globl  _call_user_but  // call the _user_but vector from C
        .globl  _call_user_wheel// call the _user_wheel vector from C
#endif


/*
 * _mousevec_handler - Mouse interrupt routine
 * entry:       a0 = address of mouse buffer from aciavecs.S
 * exit:        none
 * destroys:    nothing
 */
_mousevec_handler:
        movem.l d0-d7/a0-a6, -(sp)      // save registers used

        move.b  (a0), d0        // see if we have a mouse packet
        move.b  d0, d1          // save packet
        and.b   #0xf8, d1       // mask out for ...
        cmp.b   #0xf8, d1       // relative mouse packet header?
        jne     ms_done

        // Handle buttons
        and.w   #3, d0          // isolate mouse buttons

        lsr.b   #1, d0          // left button pressed?
        jcc     no_left         // no
        bset    #1, d0          // set bit 0 for left button
no_left:
        move.b  _cur_ms_stat,d1 // get previous mouse state
        and.w   #3,d1           // mask out state codes bits 6,7
        cmp.b   d1,d0           // has button state changed ?
        jeq     xy_update       // no: go test x,y change

        lea     _MOUSE_BT,a1
        move.w  (a1),d2
        andi.w  #0xfffc,d2
        or.w    d2,d0           // keep additional mouse button states
        move.w  d0,(a1)

        move.w  d1,-(sp)        // save previous mouse state
        move.l  _user_but,a1    // get user routine address
        jsr     (a1)
        move.w  (sp)+, d1       // get back previous mouse button state

        eor.b   d0, d1          // compute which buttons have changed
        ror.b   #2, d1          // put deltas in bits 6 & 7
        or.b    d1, d0          // combine deltas and states

        move.b  d0,_cur_ms_stat // store change in button stat

        // Handle mouse mouvement
xy_update:
        move.b  1(a0), d0       // get new relative x pos
        or.b    2(a0), d0       // get new relative y pos
        jne     new_coord       // just check for movement
        bclr.b  #5, _cur_ms_stat // no motion: clear motion status
        jra     ms_done         // no change done, bye-bye
new_coord:
        bset.b  #5, _cur_ms_stat // motion set motion status
        // Calculate new GCURX
        move.w  _GCURX, d0      // set d0 to _GCURX
        move.b  1(a0), d1
        ext.w   d1              // sign extend to word
        add.w   d1, d0          // d0 contains new x coordinate
        // Calculate new GCURY
        move.w  _GCURY,d1       // init d1 to _GCURY
        move.b  2(a0), d3
        ext.w   d3              // sign extend to word
        add.w   d3, d1          // d1 contains new x coordinate
        jbsr    scrn_clip
        // Call user routine upon motion (d0:x, d1:y)
        move.l  _user_mot,a1    // get user routine address
        jsr     (a1)            // call user to modify x,y
        // Update GCURX/GCURY
        jbsr    scrn_clip
        move.w  d0,_GCURX       // save new X location
        move.w  d1,_GCURY       // save new y location
        // Call user routine with final coordinates (d0:x, d1:y)
        move.l  _user_cur,a1    // get user routine address
        jsr     (a1)            // call user to draw cursor

                                // d0 contains new x position
                                // d1 contains new y position
ms_done:
        movem.l (sp)+,d0-d7/a0-a6  // restore registers
ms_exit:
        rts        


/* Clips the x and y-coordinates of the cursor position to the limits of the screen.
 * Input: d0.w: x, d1.w: y
 * Outputs: d0 and d1 clipped to screen size
 * Clobbered: a0 */
scrn_clip:
        lea.l   _DEV_TAB,a0     // get address of device table [0] is max_x, [1] is max_y
        tst.w   d0              // is new x-coordinate < 0?
        jge     cl_xmax         // nope - check if greater than maximum value
        clr.w   d0              // yes - set it to 0
        jra     cl_y            // now clip the y-coordinate
cl_xmax:
        cmp.w   (a0),d0         // is new x-coordinate > maximum value?
        jle     cl_y            // nope - go clip the y-coordinate
        move.w  (a0),d0         // yes - set it to the maximum value
cl_y:
        tst.w   d1              // is new y-coordinate < 0?
        jge     cl_ymax         // nope - check if greater than maximum value
        clr.w   d1              // yes - set it to 0
        rts
cl_ymax:
        cmp.w   2(a0),d1        // is new y-coordinate > maximum value?
        jle     cl_done         // nope - done
        move.w  2(a0),d1        // yes - set it to the maximum value
cl_done:
        rts


#if CONF_WITH_EXTENDED_MOUSE

/*
 * _wheel_int - Mouse wheel interrupt routine
 * entry:       a0 = address of IKBD status packet buffer from aciavecs.S
 * exit:        none
 * destroys:    nothing
 */

_wheel_int:
        movem.l d0-d1/a0-a1,-(sp)        // Save only necessary registers

        // Could this be a wheel packet?
        lea     wheel_ikbd_packet(pc),a1
        moveq   #0,d0
        moveq   #5,d1
wheel_check:
        move.b  (a0)+,d0
        cmp.b   (a1)+,d0
        dbne    d1,wheel_check
        jne     wheel_exit

        move.b  (a0)+,d0
        sub.b   #0x59,d0

        jmi     wheel_exit
        cmp.b   #3,d0
        jhi     wheel_exit

        move.w  d0,d1
        lsr.b   #1,d0                    // Wheel number

        and.w   #1,d1                    // 0 - up,  1 - down
        add.w   d1,d1                    // 0 - up,  2 - down
//      neg.w   d1                       // 0 - up, -2 - down
//      addq.w  #1,d1                    // 1 - up, -1 - down
        subq.w  #1,d1                    // -1 - up, 1 - down

        // Call user mouse wheel vector
        movem.l d2-d7/a2-a6,-(sp)        // Save remaining register to be safe
        move.l  _user_wheel,a1
        jsr     (a1)
        movem.l (sp)+,d2-d7/a2-a6

        movem.l (sp)+,d0-d1/a0-a1
        rts                              // Packet has been dealt with

wheel_exit:
        movem.l (sp)+,d0-d1/a0-a1
        move.l  _old_statvec,-(sp)       // Go to old IKBD status vector
        rts

wheel_ikbd_packet:
        .dc.b   0x05,0x00,0x00,0x00,0x00,0x00


/*
 * Call the _user_but vector from C.
 */
_call_user_but:
        move.w  4(sp),d0
        move.l  _user_but,a0
        jmp     (a0)


/*
 * Call the _user_wheel vector from C.
 */
_call_user_wheel:
        move.w  4(sp),d0
        move.w  6(sp),d1
        move.l  _user_wheel,a0
        jmp     (a0)

#endif /* CONF_WITH_EXTENDED_MOUSE */
