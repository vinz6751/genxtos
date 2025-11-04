/*
 * linea_sprite_atari.c - ATARI-specific sprite support
 *
 * Copyright 1982 by Digital Research Inc.  All rights reserved.
 * Copyright 1999 by Caldera, Inc. and Authors:
 * Copyright 2002-2022 by The EmuTOS development team
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#include "emutos.h"

#include "linea.h"
#include "lineavars.h"
/* get_start_addr() uses these: */
#include "intmath.h" /* for muls() */
#include "tosvars.h" /* for v_bas_ad, v_lin_wr */
#include "vdi_inline.h"


#define MOUSE_WIDTH     16      /* in pixels */
#define MOUSE_HEIGHT    16


#if CONF_WITH_VDI_16BIT

static void cur_display16(MCDB *sprite, MCS *mcs, WORD x, WORD y);
static void cur_replace16(MCS *mcs);

#endif /* CONF_WITH_VDI_16BIT */


static void paint_clipped_sprite(WORD op, MCDB *sprite, MCS *mcs, UWORD *mask_start, UWORD shft);


/*
 * linea_sprite_show_atari() - blits a "cursor" to the destination
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
void linea_sprite_show_atari(MCDB *sprite, MCS *mcs, WORD x, WORD y)
{
    int row_count, plane, inc, op, dst_inc;
    UWORD * addr, * mask_start;
    UWORD shft, cdb_fg, cdb_bg;
    UWORD cdb_mask;             /* for checking cdb_bg/cdb_fg */
    ULONG *save;

#if CONF_WITH_VDI_16BIT
    /*
     * handle 16-bit VDI in separate function
     */
    if (TRUECOLOR_MODE) {
        cur_display16(sprite, mcs, x, y);
        return;
    }
#endif

    x -= sprite->xhot;          /* x = left side of destination block */
    y -= sprite->yhot;          /* y = top of destination block */

    mcs->stat = 0x00;           /* reset status of save buffer */

    /*
     * clip x axis
     */
    if (x < 0) {            /* clip left */
        x += 16;                /* get address of right word */
        op = 1;                 /* remember we're clipping left */
    }
    else if (x >= (xres-15)) {  /* clip right */
        op = 2;                 /* remember we're clipping right */
    }
    else {                  /* no clipping */
        op = 0;                 /* longword save */
        mcs->stat |= MCS_LONGS; /* mark savearea as longword save */
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
    else if (y > (yres-15)) {   /* clip bottom */
        row_count = yres - y + 1;
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
     *  Store values required by cur_replace()
     */
    mcs->len = row_count;       /* number of cursor rows */
    mcs->addr = addr;           /* save area: origin of material */
    mcs->stat |= MCS_VALID;     /* flag the buffer as being loaded */

    /*
     *  To allow performance optimisations in this function, we handle
     *  L/R clipping in a separate function
     */
    if (op) {
        paint_clipped_sprite(op,sprite,mcs,mask_start,shft);
        return;
    }

    /*
     * The rest of this function handles the no-L/R clipping case
     */
    inc = v_planes;             /* # distance to next word in same plane */
    dst_inc = v_lin_wr >> 1;    /* calculate number of words in a scan line */

    save = mcs->area;           /* for long stores */

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
 * paint_clipped_sprite()
 *
 * handles cursor display for cursors that are subject to L/R clipping
 */
static void paint_clipped_sprite(WORD op, MCDB *sprite, MCS *mcs, UWORD *mask_start, UWORD shft)
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


#if CONF_WITH_VDI_16BIT
#include "vdi_defs.h"
/*
 * cur_display16() - blits mouse "cursor" to 16-bit screen
 *
 * see cur_display() for more info
 */
static void cur_display16(MCDB *sprite, MCS *mcs, WORD x, WORD y)
{
    UWORD *mask_start, *dst, *save, *palette;
    UWORD bgcol, fgcol, bgmask, fgmask;
    WORD dst_inc, i, rows, shift, width;

    /*
     * get adjusted coordinates of mouse destination
     */
    x -= sprite->xhot;
    y -= sprite->yhot;

    /*
     * figure out height, width, and where to start in mask
     */
    mask_start = sprite->maskdata;
    if (y < 0) {
        rows = y + MOUSE_HEIGHT;
        mask_start -= y * sizeof(UWORD);
        y = 0;
    } else if (y > (yres + 1 - MOUSE_HEIGHT)) {
        rows = yres + 1 - y;
    } else {
        rows = MOUSE_HEIGHT;
    }

    shift = 0;
    if (x < 0) {
        width = x + MOUSE_WIDTH;
        shift = -x;
        x = 0;
    } else if (x > (xres + 1 - MOUSE_WIDTH)) {
        width = xres + 1 - x;
    } else {
        width = MOUSE_WIDTH;
    }

    /*
     * get destination pointer & increment
     */
    dst = get_start_addr16(x, y);
    dst_inc = v_lin_wr/sizeof(UWORD) - width;

    save = (UWORD *)mcs->area;

    /*
     *  Store values required by cur_replace()
     */
    mcs->len = rows;            /* number of cursor rows */
    mcs->addr = dst;            /* save area: origin of material */
    mcs->stat |= MCS_VALID;     /* flag the buffer as being loaded */
    mcs->width = width;         /* number of cursor columns */

    /*
     * update screen, saving old contents
     */
    palette = CUR_WORK->ext->palette;
    bgcol = palette[sprite->bg_col];
    fgcol = palette[sprite->fg_col];
    while (rows-- > 0) {
        bgmask = *mask_start++;     /* set up bg mask */
        bgmask <<= shift;
        fgmask = *mask_start++;     /* set up fg mask */
        fgmask <<= shift;
        for (i = 0; i < width; i++, dst++, bgmask<<=1, fgmask<<=1) {
            *save++ = *dst;
            if (fgmask & 0x8000)
                *dst = fgcol;
            else if (bgmask & 0x8000)
                *dst = bgcol;
        }
        dst += dst_inc;
    }
}
#endif



/*
 * linea_sprite_hide_atari - replace cursor with data in save area
 *
 * note: the near-duplication of loops for the word and longword cases
 * is done deliberately for performance reasons
 *
 * input:
 *      v_planes    number of planes in destination
 *      v_lin_wr    line wrap (byte width of form)
 */
void linea_sprite_hide_atari(MCS *mcs)
{
    WORD plane, row;
    UWORD *addr, *src, *dst;
    const WORD inc = v_planes;      /* # words to next word in same plane */
    const WORD dst_inc = v_lin_wr >> 1; /* # words in a scan line */

#if CONF_WITH_VDI_16BIT
    /*
     * handle 16-bit VDI in separate function
     */
    if (TRUECOLOR_MODE) {
        cur_replace16(mcs);
        return;
    }
#endif

    if (!(mcs->stat & MCS_VALID))   /* does save area contain valid data ? */
        return;
    mcs->stat &= ~MCS_VALID;        /* yes but (like TOS) don't allow reuse */

    addr = mcs->addr;
    src = (UWORD *)mcs->area;

    /*
     * handle longword data
     */
    if (mcs->stat & MCS_LONGS) {
        /* plane controller, draw cursor in each graphic plane */
        for (plane = v_planes - 1; plane >= 0; plane--) {
            dst = addr++;           /* current destination address */
            /* loop through rows */
            for (row = mcs->len - 1; row >= 0; row--) {
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
        for (row = mcs->len - 1; row >= 0; row--) {
            *dst = *src++;
            dst += dst_inc;         /* next row of screen */
        }
    }
}


#if CONF_WITH_VDI_16BIT
/*
 * cur_replace16 - replace cursor with saved data (for 16-bit screens)
 *
 * see cur_replace for more details
 */
static void cur_replace16(MCS *mcs)
{
    UWORD *addr, *dst, *src;
    UWORD row, col;

    if (!(mcs->stat & MCS_VALID))   /* does save area contain valid data ? */
        return;
    mcs->stat &= ~MCS_VALID;        /* yes but (like TOS) don't allow reuse */

    addr = mcs->addr;               /* starting screen address */
    src = (UWORD *)mcs->area;

    for (row = mcs->len, dst = addr; row > 0; row--, dst = addr) {
        for (col = mcs->width; col > 0; col--) {
            *dst++ = *src++;
        }
        addr += v_lin_wr >> 1;
    }
}
#endif
