/*
 * linea_mouse_atari.c - ATARI-specific mouse support
 *
 * Copyright 1982 by Digital Research Inc.  All rights reserved.
 * Copyright 1999 by Caldera, Inc. and Authors:
 * Copyright 2002-2022 by The EmuTOS development team
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#include "emutos.h"

#ifndef MACHINE_A2560U

#include "asm.h"
#include "linea.h"
#include "lineavars.h"
#include "tosvars.h" /* vblqueue */
#include "xbiosbind.h" /* Initmous */


static MCS  *mcs_ptr;  /* ptr to Mouse Cursor Save area in use */

static void paint_clipped_mouse(WORD op, MCDB *sprite, MCS *mcs, UWORD *mask_start, UWORD shft);
static void paint_mouse_atari(MCDB *sprite, WORD x, WORD y);
static void unpaint_mouse_atari(void);
void mov_cur(void);             /* code to call when mouse has moved (in .S file) */

/*
 * vbl_draw - moves mouse cursor, GEM VBL routine
 *
 * It removes the mouse cursor from its current location, if necessary,
 * and redraws it at a new location.
 *
 *      Inputs:
 *         vbl_must_draw_mouse - signals need to redraw cursor
 *         newx - new cursor x-coordinate
 *         newy - new cursor y-coordinate
 *         mouse_flag - mouse cursor is being modified
 *         HIDE_CNT - mouse cursor hide/show indicator
 *
 *      Outputs:
 *         vbl_must_draw_mouse is cleared
 *
 *      Registers Modified:     d0, d1
 *
 */

/* If we do not need to draw the cursor now then just exit. */

static void vbl_draw(void)
{
    WORD old_sr, x, y;

    /* if the cursor is being modified, or is hidden, just exit */
    if (mouse_flag || HIDE_CNT)
        return;

    old_sr = set_sr(0x2700);        /* disable interrupts */
    if (vbl_must_draw_mouse) {
        vbl_must_draw_mouse = FALSE;
        x = newx;                   /* get x/y for paint_mouse() atomically */
        y = newy;
        set_sr(old_sr);
        unpaint_mouse_atari();       /* remove the old cursor from the screen */
        paint_mouse_atari(&mouse_cdb, x, y); /* display the cursor */
    } else
        set_sr(old_sr);
}


/*
 * paint_mouse() - blits a "cursor" to the destination
 *
 * before the destination is overwritten, the current contents are
 * saved to the user-provided save area (MCS).  then the cursor is
 * written, combining a background colour form, a foreground colour
 * form, and the current contents of the destination.
 *
 * some points to note:
 * the cursor is always 16x16 pixels.  in the general case, it will
 * overlap two adjacent screen words in each plane; thus the save area
 * requires 4 bytes per plane for each row of the cursor, or 64 bytes
 * in total per plane (plus some bookkeeping overhead).  if the cursor
 * is subject to left or right clipping, however, then it must lie
 * within one screen word (per plane), so we only save 32 bytes/plane.
 */
void paint_mouse_atari(MCDB *sprite, WORD x, WORD y)
{
    int row_count, plane, inc, op, dst_inc;
    UWORD * addr, * mask_start;
    UWORD shft, cdb_fg, cdb_bg;
    UWORD cdb_mask;             /* for checking cdb_bg/cdb_fg */
    ULONG *save;

    x -= sprite->xhot;          /* x = left side of destination block */
    y -= sprite->yhot;          /* y = top of destination block */

    mcs_ptr->stat = 0x00;           /* reset status of save buffer */

    /*
     * clip x axis
     */
    if (x < 0) {            /* clip left */
        x += 16;                /* get address of right word */
        op = 1;                 /* remember we're clipping left */
    }
    else if (x >= (V_REZ_HZ-15)) {  /* clip right */
        op = 2;                 /* remember we're clipping right */
    }
    else {                  /* no clipping */
        op = 0;                 /* longword save */
        mcs_ptr->stat |= MCS_LONGS; /* mark savearea as longword save */
    }

    /*
     * clip y axis
     */
    mask_start = sprite->maskdata;  /* MASK/DATA for cursor */
    if (y < 0) {            /* clip top */
        row_count = y + 16;
        mask_start -= y << 1;   /* point to first visible row of MASK/FORM */
        y = 0;                  /* and reset starting row */
    }
    else if (y > (V_REZ_VT-15)) {   /* clip bottom */
        row_count = V_REZ_VT - y + 1;
    }
    else {
        row_count = 16;
    }

    /*
     *  Compute the bit offset into the desired word, save it, and remove
     *  these bits from the x-coordinate.
     */
    addr = get_start_addr(x, y);
    shft = 16 - (x&0x0f);       /* amount to shift forms by */

    /*
     *  Store values required by unpaint_mouse()
     */
    mcs_ptr->len = row_count;       /* number of cursor rows */
    mcs_ptr->addr = addr;           /* save area: origin of material */
    mcs_ptr->stat |= MCS_VALID;     /* flag the buffer as being loaded */

    /*
     *  To allow performance optimisations in this function, we handle
     *  L/R clipping in a separate function
     */
    if (op) {
        paint_clipped_mouse(op,sprite, mcs_ptr, mask_start, shft);
        return;
    }

    /*
     * The rest of this function handles the no-L/R clipping case
     */
    inc = v_planes;             /* # distance to next word in same plane */
    dst_inc = v_lin_wr >> 1;    /* calculate number of words in a scan line */

    save = mcs_ptr->area;           /* for long stores */

    cdb_bg = sprite->bg_col;    /* get mouse background color bits */
    cdb_fg = sprite->fg_col;    /* get mouse foreground color bits */

    /* plane controller, draw cursor in each graphic plane */
    for (plane = v_planes - 1, cdb_mask = 0x0001; plane >= 0; plane--) {
        int row;
        UWORD * src, * dst;

        /* setup the things we need for each plane again */
        src = mask_start;               /* calculated mask data begin */
        dst = addr++;                   /* current destination address */

        /* loop through rows */
        for (row = row_count - 1; row >= 0; row--) {
            ULONG bits;                 /* our graphics data */
            ULONG fg;                   /* the foreground color */
            ULONG bg;                   /* the background color */

            /*
             * first, save the existing data
             */
            bits = ((ULONG)*dst) << 16; /* bring to left pos. */
            bits |= *(dst + inc);
            *save++ = bits;

            /*
             * align the forms with the cursor position on the screen
             */

            /* get and align background & foreground forms */
            bg = (ULONG)*src++ << shft;
            fg = (ULONG)*src++ << shft;

            /*
             * logical operation for cursor interaction with screen
             * note that this only implements the "VDI" mode
             */

            /* select operation for mouse mask background color */
            if (cdb_bg & cdb_mask)
                bits |= bg;
            else
                bits &= ~bg;

            /* select operation for mouse mask foreground color */
            if (cdb_fg & cdb_mask)
                bits |= fg;
            else
                bits &= ~fg;

            /*
             * update the screen with the new data
             */
            *dst = (UWORD)(bits >> 16);
            *(dst + inc) = (UWORD)bits;
            dst += dst_inc;             /* next row of screen */
        } /* loop through rows */

        cdb_mask <<= 1;
    } /* loop through planes */
}


/*
 * paint_clipped_mouse()
 *
 * handles cursor display for cursors that are subject to L/R clipping
 */
static void paint_clipped_mouse(WORD op,MCDB *sprite,MCS *mcs,UWORD *mask_start,UWORD shft)
{
    WORD dst_inc, plane;
    UWORD cdb_fg, cdb_bg;
    UWORD cdb_mask;             /* for checking cdb_bg/cdb_fg */
    UWORD *addr, *save;

    dst_inc = v_lin_wr >> 1;    /* calculate number of words in a scan line */

    addr = mcs->addr;           /* starting screen address */
    save = (UWORD *)mcs->area;  /* we save words, not longwords */

    cdb_bg = sprite->bg_col;    /* get mouse background color bits */
    cdb_fg = sprite->fg_col;    /* get mouse foreground color bits */

    /* plane controller, draw cursor in each graphic plane */
    for (plane = v_planes - 1, cdb_mask = 0x0001; plane >= 0; plane--) {
        WORD row;
        UWORD *src, *dst;

        /* setup the things we need for each plane again */
        src = mask_start;               /* calculated mask data begin */
        dst = addr++;                   /* current destination address */

        /* loop through rows */
        for (row = mcs->len - 1; row >= 0; row--) {
            ULONG bits;                 /* our graphics data */
            ULONG fg;                   /* the foreground color */
            ULONG bg;                   /* the background color */

            /*
             * first, save the existing data
             */
            *save++ = *dst;
            if (op == 1) {          /* right word only */
                bits = *dst;            /* dst already at right word */
            } else {                /* left word only  */
                bits = ((ULONG)*dst) << 16; /* move to left posn */
            }

            /*
             * align the forms with the cursor position on the screen
             */

            /* get and align background & foreground forms */
            bg = (ULONG)*src++ << shft;
            fg = (ULONG)*src++ << shft;

            /*
             * logical operation for cursor interaction with screen
             */

            /* select operation for mouse mask background color */
            if (cdb_bg & cdb_mask)
                bits |= bg;
            else
                bits &= ~bg;

            /* select operation for mouse mask foreground color */
            if (cdb_fg & cdb_mask)
                bits |= fg;
            else
                bits &= ~fg;

            /*
             * update the screen with the new data
             */
            if (op == 1) {          /* right word only */
                *dst = (UWORD)bits;
            } else {                /* left word only */
                *dst = (UWORD)(bits >> 16);
            }

            dst += dst_inc;             /* a1 -> next row of screen */
        } /* loop through rows */

        cdb_mask <<= 1;
    } /* loop through planes */
}


/*
 * unpaint_mouse - replace cursor with data in save area
 *
 * note: the near-duplication of loops for the word and longword cases
 * is done deliberately for performance reasons
 *
 * input:
 *      v_planes    number of planes in destination
 *      v_lin_wr    line wrap (byte width of form)
 */
void unpaint_mouse_atari(void)
{
    WORD plane, row;
    UWORD *addr, *src, *dst;
    const WORD inc = v_planes;      /* # words to next word in same plane */
    const WORD dst_inc = v_lin_wr >> 1; /* # words in a scan line */

    if (!(mcs_ptr->stat & MCS_VALID))   /* does save area contain valid data ? */
        return;
    mcs_ptr->stat &= ~MCS_VALID;        /* yes but (like TOS) don't allow reuse */

    addr = mcs_ptr->addr;
    src = (UWORD *)mcs_ptr->area;

    /*
     * handle longword data
     */
    if (mcs_ptr->stat & MCS_LONGS) {
        /* plane controller, draw cursor in each graphic plane */
        for (plane = v_planes - 1; plane >= 0; plane--) {
            dst = addr++;           /* current destination address */
            /* loop through rows */
            for (row = mcs_ptr->len - 1; row >= 0; row--) {
                *dst = *src++;
                *(dst + inc) = *src++;
                dst += dst_inc;     /* next row of screen */
            }
        }
        return;
    }

    /*
     * handle word data
     */

    /* plane controller, draw cursor in each graphic plane */
    for (plane = v_planes - 1; plane >= 0; plane--) {
        dst = addr++;               /* current destination address */
        /* loop through rows */
        for (row = mcs_ptr->len - 1; row >= 0; row--) {
            *dst = *src++;
            dst += dst_inc;         /* next row of screen */
        }
    }
}


static void set_mouse_cursor(const MFORM *src) {
    int i;
    UWORD * gmdt;                /* global mouse definition table */
    MCDB *dst = &mouse_cdb;
    const UWORD * mask;
    const UWORD * data;
    
    /* save x-offset of mouse hot spot */
    dst->xhot = src->mf_xhot & 0x000f;

    /* save y-offset of mouse hot spot */
    dst->yhot = src->mf_yhot & 0x000f;

    /*
     * Move the new mouse definition into the global mouse cursor definition
     * table.  The values for the mouse mask and data are supplied as two
     * separate 16-word entities.  They must be stored as a single array
     * starting with the first word of the mask followed by the first word
     * of the data and so on.
     */

    /* copy the data to the global mouse definition table */
    gmdt = dst->maskdata;
    mask = src->mf_mask;
    data = src->mf_data;
    for (i = 15; i >= 0; i--) {
        *gmdt++ = *mask++;              /* get next word of mask */
        *gmdt++ = *data++;              /* get next word of data */
    }    
}


static void resolution_changed(void)
{
# if EXTENDED_PALETTE
    mcs_ptr = (v_planes <= 4) ? &mouse_cursor_save : &ext_mouse_cursor_save;
# else
    mcs_ptr = &mouse_cursor_save;
# endif    
}


static void init(void)
{
    vblqueue[0] = vbl_draw;      /* set GEM VBL-routine to the first VBL slot */
    user_cur = mov_cur;         /* initialize user_cur vector */    
}


static void deinit(void)
{
    vblqueue[0] = vbl_draw;      /* set GEM VBL-routine to the first VBL slot */

    /* disable mouse via XBIOS */
    Initmous(0, 0, 0);
}


const LINEA_MOUSE_RENDERER mouse_display_driver = {
    init,
    deinit,
    vbl_draw,
    paint_mouse_atari,
    unpaint_mouse_atari,
    set_mouse_cursor,
    resolution_changed
};

#endif