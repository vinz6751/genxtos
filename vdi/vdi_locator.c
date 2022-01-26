/*
 * vdi_locator.c - VDI mouse-related functions
 *
 * Copyright 1982 by Digital Research Inc.  All rights reserved.
 * Copyright 1999 by Caldera, Inc. and Authors:
 * Copyright 2002-2021 by The EmuTOS development team
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#include "emutos.h"
#include "asm.h"
#include "biosbind.h"
#include "lineavars.h"
#include "linea.h"
#include "vdi_defs.h"

/*
 * gloc_key - get locator key
 *
 * returns:  0    - nothing
 *           1    - button pressed
 *                  TERM_CH = 16 bit char info
 *           2    - coordinate info
 *                     X1 = new x
 *                     Y1 = new y
 *
 * The variable cur_ms_stat holds the bitmap of mouse status since the last
 * interrupt. The bits are
 *
 * 0 - 0x01 Left mouse button status  (0=up)
 * 1 - 0x02 Right mouse button status (0=up)
 * 2 - 0x04 Reserved
 * 3 - 0x08 Reserved
 * 4 - 0x10 Reserved
 * 5 - 0x20 Mouse move flag (1=moved)
 * 6 - 0x40 Right mouse button status flag (0=hasn't changed)
 * 7 - 0x80 Left mouse button status flag  (0=hasn't changed)
 */
static WORD gloc_key(void)
{
    WORD retval = 0;
    ULONG ch;

    /*
     * check for mouse button or keyboard key
     */
    if (cur_ms_stat & 0xc0) {           /* some button status bits set? */
        if (cur_ms_stat & 0x40)         /* if bit 6 set,                     */
            TERM_CH = 0x20;             /* send terminator code for left key */
        else
            TERM_CH = 0x21;             /* send terminator code for right key */
        cur_ms_stat &= 0x23;            /* clear mouse button status (bit 6/7) */
        retval = 1;                     /* set button pressed flag */
    } else if (Bconstat(2)) {           /* see if a character present at con */
        ch = Bconin(2);
        TERM_CH = (WORD)
                  (ch >> 8)|            /* scancode down to bit 8-15 */
                  (ch & 0xff);          /* asciicode to bit 0-7 */
        retval = 1;                     /* set button pressed flag */
    }

    /*
     * check for mouse movement
     */
    if (cur_ms_stat & 0x20) {           /* if bit #5 set ... */
        Point * point = (Point*)PTSIN;

        cur_ms_stat &= ~0x20;   /* clear bit 5 */
        point->x = GCURX;       /* set X = GCURX */
        point->y = GCURY;       /* set Y = GCURY */
        retval += 2;
    }

    return retval;
}



/*
 * LOCATOR_INPUT: implements vrq_locator()/vsm_locator()
 *
 * These functions return the status of the logical 'locator' device.
 *
 * vrq_locator() operation in Atari TOS and EmuTOS
 * -----------------------------------------------
 * 1. The first call to vrq_locator() always returns immediately: the
 *    output mouse positions are the same as the input, and the
 *    terminating character is set to 0x20, indicating the left mouse
 *    button.
 * 2. Subsequent calls return when either a keyboard key is pressed, or
 *    a mouse button is pressed OR released (thus a normal mouse button
 *    action satisfies TWO calls to vrq_locator()).  The output mouse
 *    positions are the current positions, and the terminating character
 *    is the ASCII key pressed, or 0x20 for the left mouse button / 0x21
 *    for the right.
 *    As a consequence, pressing the space key twice is indistinguishable
 *    from pressing/releasing the left mouse button, and likewise for
 *    the exclamation mark and the right mouse button.
 *
 * vsm_locator() operation in Atari TOS and EmuTOS
 * -----------------------------------------------
 * 1. The first call to vsm_locator() always sets the terminating
 *    character to 0x20 and CONTRL[4] to 1 (indicating the left mouse
 *    button).
 * 2. On every call:
 *    . if the mouse has been moved, CONTRL[2] is set to 1
 *    . if a keyboard key is pressed, the terminating character is the
 *      ASCII value of the key pressed, and CONTRL[4] is set to 1
 *    . if a mouse button is pressed or released, the terminating
 *      character is 0x20 for the left button, 0x21 for the right
 *      button, and CONTRL[4] is set to 1
 *    . the output mouse positions are always set to the same as the
 *      input
 *
 * Differences from official Atari documentation
 * ---------------------------------------------
 * 1. No special behaviour is described for the first call to
 *    vrq_locator() or vsm_locator().
 * 2. No mention is made of button press & release being separate
 *    events.
 * 3. For vsm_locator(), the output mouse positions should be the
 *    current positions, not the input positions.
 */
void vdi_v_locator(Vwk * vwk)
{
    WORD i;
    Point * point = (Point*)PTSIN;

    /* Set the initial locator position. */
    GCURX = point->x;
    GCURY = point->y;

    if (loc_mode == 0) {    /* handle request mode (vrq_locator()) */
        linea_mouse_show();
        /* loop till button or keyboard event */
        while (!(gloc_key() & 1)) {
        }
        INTOUT[0] = TERM_CH & 0x00ff;

        CONTRL[4] = 1;
        CONTRL[2] = 1;

        PTSOUT[0] = point->x;
        PTSOUT[1] = point->y;
        linea_mouse_hide();
    } else {                /* handle sample mode (vsm_locator()) */
        i = gloc_key();
        if (i & 1) {
            CONTRL[4] = 1;
            INTOUT[0] = TERM_CH & 0x00ff;
        }
        if (i & 2) {
            CONTRL[2] = 1;
            PTSOUT[0] = point->x;
            PTSOUT[1] = point->y;
        }
    }
}



/*
 * vdi_v_show_c - show cursor
 */
void vdi_v_show_c(Vwk * vwk)
{
    linea_mouse_force_show();
}



/*
 * vdi_v_hide_c - hide cursor
 */
void vdi_v_hide_c(Vwk * vwk)
{
    linea_mouse_hide();
}



/*
 * vdi_vq_mouse - Query mouse position and button status
 */
void vdi_vq_mouse(Vwk * vwk)
{
    WORD old_sr;

    old_sr = set_sr(0x2700);    /* disable interrupts */

    INTOUT[0] = MOUSE_BT;
    PTSOUT[0] = GCURX;
    PTSOUT[1] = GCURY;

    set_sr(old_sr);             /* enable interrupts */
}



/*
 * vdi_vex_butv
 *
 * This routine replaces the mouse button change vector with
 * the address of a user-supplied routine.  The previous value
 * is returned so that it also may be called when there is a
 * change in the mouse button status.
 *
 * Inputs:
 *    contrl[7], contrl[8] - pointer to user routine
 *
 * Outputs:
 *    contrl[9], contrl[10] - pointer to old routine
 */
void vdi_vex_butv(Vwk * vwk)
{
    ULONG_AT(&CONTRL[9]) = (ULONG) user_but;
    user_but = (PFVOID) ULONG_AT(&CONTRL[7]);
}



/*
 * vdi_vex_motv
 *
 * This routine replaces the mouse coordinate change vector with the address
 * of a user-supplied routine.  The previous value is returned so that it
 * also may be called when there is a change in the mouse coordinates.
 *
 *  Inputs:
 *     contrl[7], contrl[8] - pointer to user routine
 *
 *  Outputs:
 *     contrl[9], contrl[10] - pointer to old routine
 */
void vdi_vex_motv(Vwk * vwk)
{
    ULONG_AT(&CONTRL[9]) = (ULONG) user_mot;
    user_mot = (PFVOID) ULONG_AT(&CONTRL[7]);
}



/*
 * vdi_vex_curv
 *
 * This routine replaces the mouse draw vector with the
 * address of a user-supplied routine.  The previous value
 * is returned so that it also may be called when the mouse
 * is to be drawn.
 *
 * Inputs:
 *    contrl[7], contrl[8] - pointer to user routine
 *
 * Outputs:
 *    contrl[9], contrl[10] - pointer to old routine
 *
 */
void vdi_vex_curv(Vwk * vwk)
{
    ULONG_AT(&CONTRL[9]) = (ULONG) user_cur;
    user_cur = (PFVOID) ULONG_AT(&CONTRL[7]);
}


/*
 * vdi_vsc_form - Transforms user defined mouse cursor to device specific format.
 *
 * Get the new values for the x and y-coordinates of the mouse hot
 * spot and the new color indices for the mouse mask and data.
 *
 * Inputs:
 *     intin[0] - x coordinate of hot spot
 *     intin[1] - y coordinate of hot spot
 *     intin[2] - reserved for future use. must be 1
 *     intin[3] - Mask color index
 *     intin[4] - Data color index
 *     intin[5-20]  - 16 words of cursor mask
 *     intin[21-36] - 16 words of cursor data
 *
 * Outputs:        None
 */
void vdi_vsc_form(Vwk * vwk)
{
    linea_mouse_transform();
}
