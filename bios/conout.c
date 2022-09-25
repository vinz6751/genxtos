/*
 * conout.c - lowlevel color model dependent screen handling routines
 *
 *
 * Copyright (C) 2004 by Authors (see below)
 * Copyright (C) 2016-2020 The EmuTOS development team
 *
 * Authors:
 *  MAD     Martin Doering
 *  VB      Vincent Barrilliot
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

/*
 * NOTE: the code currently assumes that the font width is 8 bits.
 * If we ever add a 16x32 font, the code will need changing!
 */

/* #define ENABLE_KDEBUG */

#include "emutos.h"
#include "asm.h"
#include "lineavars.h"
#include "tosvars.h"            /* for v_bas_ad */
#include "string.h"
#include "conout.h"
#include "font.h"
#include "a2560u_bios.h"

/* Driver in use for outputing text */
static CONOUT_DRIVER *conout;

#if !defined(MACHINE_A2560U)
extern const CONOUT_DRIVER conout_atarifb;
#endif


void conout_init(const Fonthead *font)
{    
    v_cel_mx = (V_REZ_HZ / font->max_cell_width) - 1;
    v_cel_my = (V_REZ_VT / font->form_height) - 1;

#ifdef MACHINE_A2560U
    conout = a2560u_bios_get_conout();
#else
    conout = &conout_atarifb;
#endif
    KDEBUG(("conout_init calling init of driver\n"));
    conout->init(font);

    KDEBUG(("conout_init exiting\n"));
}


/*
 * cell_addr - convert cell X,Y to a screen address.
 *
 *
 * convert cell X,Y to a screen address. also clip cartesian coordinates
 * to the limits of the current screen.
 */

static CHAR_ADDR cell_addr(UWORD x, UWORD y)
{
    /* check bounds against screen limits */
    if (x > v_cel_mx)
        x = v_cel_mx;           /* clipped x */

    if (y > v_cel_my)
        y = v_cel_my;           /* clipped y */

    return conout->cell_addr(x,y);
}


/*
 * neg_cell - negates
 *
 * This routine negates the contents of an arbitrarily-tall byte-wide cell
 * composed of an arbitrary number of (Atari-style) bit-planes.
 * Cursor display can be accomplished via this procedure.  Since a second
 * negation restores the original cell condition, there is no need to save
 * the contents beneath the cursor block.
 *
 * in:
 * a1.l      points to destination (1st plane, top of block)
 *
 * out:
 */

static void neg_cell(CHAR_ADDR cell)
{
    v_stat_0 |= M_CRIT;                 /* start of critical section. */

    conout->neg_cell(cell);

    v_stat_0 &= ~M_CRIT;                /* end of critical section. */
}


/*
 * next_cell - Return the next cell address.
 *
 * sets next cell address given the current position and screen constraints
 *
 * returns:
 *     false - no wrap condition exists
 *     true  - CR LF required (position has not been updated)
 */

static BOOL next_cell(void)
{
    /* check bounds against screen limits */
    if (v_cur_cx == v_cel_mx) {         /* increment cell ptr */
        if (!(v_stat_0 & M_CEOL)) {
            /* overwrite in effect */
            return 0;                   /* no wrap condition exists */
                                        /* don't change cell parameters */
        }

        /* call carriage return routine */
        /* call line feed routine */
        return 1;                       /* indicate that CR LF is required */
    }

    v_cur_cx += 1;                      /* next cell to right */

    conout->next_cell();

    return 0;                           /* indicate no wrap needed */
}


/*
 * conout_invert_cell - negates the cells bits
 *
 * This routine negates the contents of an arbitrarily-tall byte-wide cell
 * composed of an arbitrary number of (Atari-style) bit-planes.
 *
 * Wrapper for neg_cell().
 *
 * in:
 * x - cell X coordinate
 * y - cell Y coordinate
 */

void conout_invert_cell(int x, int y)
{
    /* fetch x and y coords and invert cursor. */
    conout->neg_cell(cell_addr(x, y));
}


/*
 * conout_move_cursor - move the cursor.
 *
 * move the cursor and update global parameters
 * erase the old cursor (if necessary) and draw new cursor (if necessary)
 *
 * in:
 * d0.w    new cell X coordinate
 * d1.w    new cell Y coordinate
 */

void conout_move_cursor(int x, int y)
{
    /* update cell position */

    /* clamp x,y to valid ranges */
    if (x < 0)
        x = 0;
    else if (x > v_cel_mx)
        x = v_cel_mx;

    if (y < 0)
        y = 0;
    else if (y > v_cel_my)
        y = v_cel_my;

    v_cur_cx = x;
    v_cur_cy = y;
#if 1
    if (conout->cursor_moved)
        conout->cursor_moved();

    if (v_stat_0 & M_CVIS) {
        /* is cursor flashing? */
        if (v_stat_0 & M_CFLASH) {
            v_stat_0 &= ~M_CVIS;  /* yes, make invisible...semaphore. */

            /* if cursor flashing enabled and cursor is presently painted, unpaint */
            if (v_stat_0 & M_CSTATE)
                conout_unpaint_cursor();

            /* set new coordinates and paint there */
            v_cur_ad = cell_addr(x, y);
            conout_paint_cursor();
        }
    }
    else {
        /* not visible */
        v_cur_ad = cell_addr(x, y);             /* just set new coordinates */
    }  
#else
    /* is cursor visible? */
    if (!(v_stat_0 & M_CVIS)) {
        /* not visible */
        v_cur_ad = cell_addr(x, y);             /* just set new coordinates */
        return;                                 /* and quit */
    }

    /* is cursor flashing? */
    if (v_stat_0 & M_CFLASH) {
        v_stat_0 &= ~M_CVIS;                    /* yes, make invisible...semaphore. */

        /* is cursor presently displayed ? */
        if (!(v_stat_0 & M_CSTATE)) {
            /* not displayed */
            v_cur_ad = cell_addr(x, y);         /* just set new coordinates */

            /* show the cursor when it moves */
            neg_cell(v_cur_ad);                 /* complement cursor. */
            v_stat_0 |= M_CSTATE;
            v_cur_tim = v_period;               /* reset the timer. */

            v_stat_0 |= M_CVIS;                 /* end of critical section. */
            return;
        }
    }

    /* move the cursor after all special checks failed */
    neg_cell(v_cur_ad);                         /* erase present cursor */

    v_cur_ad = cell_addr(x, y);                 /* fetch x and y coords. */
    neg_cell(v_cur_ad);                         /* complement cursor. */

    /* do not flash the cursor when it moves */
    v_cur_tim = v_period;                       /* reset the timer. */

    v_stat_0 |= M_CVIS;                         /* end of critical section. */

#endif
}



/*
 * conout_ascii_out - prints an ascii character on the screen
 *
 * in:
 *
 * ch.w      ascii code for character
 */

void conout_ascii_out(int ch)
{
    CHAR_ADDR src;
    CHAR_ADDR dst;
    BOOL visible;                       /* was the cursor visible? */

    if (!conout->get_char_source(ch, &src))
        return;

    dst = v_cur_ad;                     /* a1 -> get destination */

    visible = v_stat_0 & M_CVIS;        /* test visibility bit */
    if (visible)
        v_stat_0 &= ~M_CVIS;            /* start of critical section */

    /* put the cell out (this covers the cursor) */    
    conout->cell_xfer(src, dst);

    /* advance the cursor and update cursor address and coordinates */
    if (next_cell()) {
        /* CRLF */
        CHAR_ADDR cell;
        UWORD y = v_cur_cy;

        /* perform cell carriage return. */        
        v_cur_cx = 0;                   /* set X to first cell in line */

        /* perform cell line feed. */
        if (y < v_cel_my) {
            cell = conout->cell_addr(0, y + v_cel_wr); /* move down one cell */
            v_cur_cy = y + 1;           /* update cursor's y coordinate */
        }
        else {
            cell = conout->cell_addr(0, y); /* cursor stays on (current) last line */
            conout_scroll_up(0);            /* scroll from top of screen */
        }
        v_cur_ad = cell;                /* update cursor address */   
    }

    if (conout->cursor_moved)
        conout->cursor_moved();

    /* if visible */
    if (visible) {
#if 1
        conout_paint_cursor();             /* display cursor. */
#else
        neg_cell(v_cur_ad);             /* display cursor. */
        v_stat_0 |= M_CSTATE;           /* set state flag (cursor on). */
        v_stat_0 |= M_CVIS;             /* end of critical section. */

        /* do not flash the cursor when it moves */
        if (v_stat_0 & M_CFLASH) {
            v_cur_tim = v_period;       /* reset the timer. */
        }
#endif
    }
}


void conout_blank_out(int topx, int topy, int botx, int boty)
{
    conout->blank_out(topx, topy, botx, boty);
}



/*
 * conout_scroll_up - Scroll upwards
 *
 *
 * Scroll copies a source region as wide as the screen to an overlapping
 * destination region on a one cell-height offset basis.  Two entry points
 * are provided:  Partial-lower scroll-up, partial-lower scroll-down.
 * Partial-lower screen operations require the cell y # indicating the
 * top line where scrolling will take place.
 *
 * After the copy is performed, any non-overlapping area of the previous
 * source region is "erased" by calling conout_blank_out which fills the area
 * with the background color.
 *
 * in:
 *   top_line - cell y of cell line to be used as top line in scroll
 */

void conout_scroll_up(UWORD top_line)
{
    ULONG count;
    CHAR_ADDR src;
    CHAR_ADDR dst;

    dst = conout->cell_addr(0, top_line);
    src = conout->cell_addr(0, top_line + 1);
    count = (ULONG)v_cel_wr * (v_cel_my - top_line);
    conout->scroll_up(src, dst, count);   
}



/*
 * conout_scroll_down - Scroll (partially) downwards
 */

void conout_scroll_down(UWORD start_line)
{
    ULONG count;
    CHAR_ADDR src;
    CHAR_ADDR dst;

    src = cell_addr(0, start_line);
    dst = cell_addr(0, start_line + 1);
    count = (ULONG)v_cel_wr * (v_cel_my - start_line);

    conout->scroll_down(src, dst, count, start_line);
}


void conout_paint_cursor(void)
{
    v_stat_0 |= M_CSTATE;

    conout->con_paint_cursor();

    v_cur_tim = v_period; /* reset the timer for blinking */
    v_stat_0 |= M_CVIS;   /* set visibility bit */
}


void conout_unpaint_cursor(void)
{
    v_stat_0 &= ~M_CSTATE;

    conout->unpaint_cursor();
}


/*
 * blink - cursor blink interrupt routine
 *
 * This used to be in vt52.c where it arguably belong but since the behaviour depends
 * on the underlying conout driver.
 */
void conout_blink_cursor(void)
{
    if (conout->blink_cursor)
    {
        conout->blink_cursor();
    }
    else
    {
        /* test visibility/semaphore bit */
        if (!(v_stat_0 & M_CVIS) )
            return;    /* if invisible or blocked, return */

        /* test flash bit */
        if (!(v_stat_0 & M_CFLASH) )
            return;    /* if not flashing, return */

        /* decrement cursor flash timer */
        if ( --v_cur_tim )
            return;    /* if <> 0, return */

        v_cur_tim = v_period;       /* else reset timer */

        /* toggle cursor state */
        if ( v_stat_0 & M_CSTATE ) {
            conout_unpaint_cursor();
        }
        else {
            conout_paint_cursor();
        }
    }
}