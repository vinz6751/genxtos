/*
 * conout.c - lowlevel color model dependent screen handling routines
 * for the Atari frame buffer
 *
 *
 * Copyright (C) 2004 by Authors (see below)
 * Copyright (C) 2016-2020 The EmuTOS development team
 *
 * Authors:
 *  MAD     Martin Doering
 *  VB      Vincent Barrilliot
 *  RB      Roger Burrows
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

#if !defined(MACHINE_A2560U) && !defined(MACHINE_A2560X)

#include "asm.h"
#include "lineavars.h"
#include "tosvars.h"            /* for v_bas_ad */
#include "sound.h"              /* for bell() */
#include "string.h"
#include "conout.h"
#include "font.h"
#include "a2560u_bios.h"
#include "../vdi/vdi_defs.h"    /* for phys_work stuff */

#define PLANE_OFFSET    2       /* interleaved planes */

#define FALCON_BLACK    0x0000
#define FALCON_WHITE    0xffbf

#if CONF_WITH_VIDEL
static const UWORD falcon_default_palette[16] = {
    0xffdf, 0xf800, 0x07c0, 0xffc0, 0x001f, 0xf81f, 0x07df, 0xbdd7,
    0x8c51, 0xa800, 0x0540, 0xad40, 0x0015, 0xa815, 0x0555, FALCON_BLACK
};
#endif

#if CONF_WITH_VDI_16BIT
extern Vwk phys_work;           /* attribute area for physical workstation */
#endif

static void init(const Fonthead *font)
{
    v_cel_mx = (V_REZ_HZ / font->max_cell_width) - 1;
    v_cel_my = (V_REZ_VT / font->form_height) - 1;
    v_cel_wr = v_lin_wr * v_cel_ht;
    v_cur_ad.pxaddr = v_bas_ad;                /* set cursor to start of screen */
}


/*
 * cell_addr - convert cell X,Y to a screen address.
 *
 * convert cell X,Y to a screen address. also clip cartesian coordinates
 * to the limits of the current screen.
 *
 * input:
 *  x       cell X
 *  y       cell Y
 *
 * returns pointer to first byte of cell
 */

static CHAR_ADDR cell_addr(UWORD x, UWORD y)
{
    ULONG disx, disy;

#if CONF_WITH_VIDEL
    if (TRUECOLOR_MODE) {       /* chunky pixels */
        disx = v_planes * x;
    }
    else
#endif
    {
        /*
         * v_planes cannot be more than 8, so as long as there are no more
         * than 4000 characters per line, the result will fit in a word ...
         *
         * X displacement = even(X) * v_planes + Xmod2
         */
        disx = v_planes * (x & ~1);
        if (IS_ODD(x)) {        /* Xmod2 = 0 ? */
            disx++;             /* Xmod2 = 1 */
        }
    }

    /* Y displacement = Y // cell conversion factor */
    disy = (ULONG)v_cel_wr * y;

    /*
     * cell address = screen base address + Y displacement
     * + X displacement + offset from screen-begin (fix)
     */
    return (CHAR_ADDR)(v_bas_ad + disy + disx + v_cur_of);
}


static int get_char_source(unsigned char c, CHAR_ADDR *src)
{
    (*src).pxaddr = char_addr(c);   /* a0 -> get character source */
    return src != NULL;             /* return false if char not in font */
}


#if CONF_WITH_VIDEL
/*
 * cell_xfer16 - cell_xfer() for Falcon 16-bit graphics
 *
 * see the comments in cell_xfer() for more details
 */
static void cell_xfer16(UBYTE *src, UBYTE *dst)
{
    UWORD *p;
    UWORD fg, fgcol;
    UWORD bg, bgcol;
    WORD fnt_wr, line_wr, i, mask;

    MAYBE_UNUSED(fg);
    MAYBE_UNUSED(bg);

    fnt_wr = v_fnt_wr;
    line_wr = v_lin_wr;

    if (v_stat_0 & M_REVID) {   /* handle reversed foreground and background colours */
        fg = v_col_bg;
        bg = v_col_fg;
    } else {
        fg = v_col_fg;
        bg = v_col_bg;
    }

    /*
     * if we have 16-bit support in VDI and if the VDI workstation is initialized,
     * we use its palette, otherwise, e.g. at boot, we use a default palette.
     */
#if CONF_WITH_VDI_16BIT
    if (phys_work.ext) {
        fgcol = phys_work.ext->palette[fg];
        bgcol = phys_work.ext->palette[bg];
    } else
#endif
    {
        fgcol = falcon_default_palette[fg & 0xf];
        bgcol = falcon_default_palette[bg & 0xf];
    }

    for (i = v_cel_ht; i--; ) {
        for (mask = 0x80, p = (UWORD *)dst; mask; mask >>= 1) {
            *p++ = (*src & mask) ? fgcol : bgcol;
        }
        dst += line_wr;
        src += fnt_wr;
    }
}
#endif


/*
 * cell_xfer - Performs a byte aligned block transfer.
 *
 *
 * This routine performs a byte aligned block transfer for the purpose of
 * manipulating monospaced byte-wide text. the routine maps a single-plane,
 * arbitrarily-long byte-wide image to a multi-plane bit map.
 * all transfers are byte aligned.
 *
 * in:
 * a0.l      points to contiguous source block (1 byte wide)
 * a1.l      points to destination (1st plane, top of block)
 *
 * out:
 * a4      points to byte below this cell's bottom
 */

static void cell_xfer(CHAR_ADDR src, CHAR_ADDR dst)
{
    UBYTE * src_sav, * dst_sav;
    UWORD fg;
    UWORD bg;
    int fnt_wr, line_wr;
    int plane;

#if CONF_WITH_VIDEL
    if (TRUECOLOR_MODE) {
        cell_xfer16(src.pxaddr, dst.pxaddr);
        return;
    }
#endif

    fnt_wr = v_fnt_wr;
    line_wr = v_lin_wr;

    /* check for reversed foreground and background colors */
    if (v_stat_0 & M_REVID) {
        fg = v_col_bg;
        bg = v_col_fg;
    }
    else {
        fg = v_col_fg;
        bg = v_col_bg;
    }

    src_sav = src.pxaddr;
    dst_sav = dst.pxaddr;

    for (plane = v_planes; plane--; ) {
        int i;

        src.pxaddr = src_sav;                  /* reload src */
        dst.pxaddr = dst_sav;                  /* reload dst */

        if (bg & 0x0001) {
            if (fg & 0x0001) {
                /* back:1  fore:1  =>  all ones */
                for (i = v_cel_ht; i--; ) {
                    *dst.pxaddr = 0xff;                /* inject a block */
                    dst.pxaddr += line_wr;
                }
            }
            else {
                /* back:1  fore:0  =>  invert block */
                for (i = v_cel_ht; i--; ) {
                    /* inject the inverted source block */
                    *dst.pxaddr = ~*src.pxaddr;
                    dst.pxaddr += line_wr;
                    src.pxaddr += fnt_wr;
                }
            }
        }
        else {
            if (fg & 0x0001) {
                /* back:0  fore:1  =>  direct substitution */
                for (i = v_cel_ht; i--; ) {
                    *dst.pxaddr = *src.pxaddr;
                    dst.pxaddr += line_wr;
                    src.pxaddr += fnt_wr;
                }
            }
            else {
                /* back:0  fore:0  =>  all zeros */
                for (i = v_cel_ht; i--; ) {
                    *dst.pxaddr = 0x00;                /* inject a block */
                    dst.pxaddr += line_wr;
                }
            }
        }

        bg >>= 1;                       /* next background color bit */
        fg >>= 1;                       /* next foreground color bit */
        dst_sav += PLANE_OFFSET;        /* top of block in next plane */
    }
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
    int len;
    int plane;
    int lin_wr = v_lin_wr;
    int cell_len = v_cel_ht;

#if CONF_WITH_VIDEL
    if (TRUECOLOR_MODE) {               /* chunky pixels */
        for (len = cell_len; len--; ) {
            WORD i;
            UWORD *addr;
            for (i = 8, addr = (UWORD *)cell.pxaddr; i--; addr++)
                *addr = ~*addr;
            cell.pxaddr += lin_wr;
        }
    }
    else
#endif
    {
        for (plane = v_planes; plane--; ) {
            UBYTE * addr = cell.pxaddr;            /* top of current dest plane */

            /* reset cell length counter */
            for (len = cell_len; len--; ) {
                *addr = ~*addr;
                addr += lin_wr;
            }
            cell.pxaddr += PLANE_OFFSET;           /* a1 -> top of block in next plane */
        }
    }
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

static void next_cell(void)
{
#if CONF_WITH_VIDEL
    if (TRUECOLOR_MODE) {               /* chunky pixels */
        v_cur_ad.pxaddr += 16;
        return;
    }
#endif

    /* if X is even, move to next word in the plane */
    if (IS_ODD(v_cur_cx)) {
        /* x is odd */
        v_cur_ad.pxaddr += 1;           /* a1 -> new cell */
        return;
    }

    /* new cell (1st plane), added offset to next word in plane */
    v_cur_ad.pxaddr += (v_planes << 1) - 1;
}


#if CONF_WITH_VIDEL
/*
 * blank_out16 - blank_out() for Falcon 16-bit graphics
 *
 * see the header comments in blank_out() for more details
 */
static void blank_out16(int topx, int topy, int botx, int boty)
{
    UWORD *addr;
    UWORD bgcol;
    WORD i, j;
    WORD offs, rows, width;

    width = (botx - topx + 1) * 8;          /* in words */

    /* calculate the offset from the end of row to next row start */
    offs = v_lin_wr/sizeof(WORD) - width;   /* in words */

    rows = (boty - topy + 1) * v_cel_ht;    /* in pixels */

    /* set standard background colour */
    bgcol = falcon_default_palette[v_col_bg & 0xf];

#if CONF_WITH_VDI_16BIT
    /*
     * if we're already in 16-bit mode, we can get the pixel value of
     * the background colour from the physical workstation's palette instead
     */
    if (phys_work.ext)
        bgcol = phys_work.ext->palette[v_col_bg];
#endif

    addr = (UWORD *)cell_addr(topx, topy).pxaddr;  /* running pointer to screen */
    for (i = 0; i < rows; i++) {
        for (j = 0; j < width; j++) {
            *addr++ = bgcol;
        }
        addr += offs;
    }
}
#endif


/*
 * blank_out - Fills region with the background color.
 *
 * Fills a cell-word aligned region with the background color.
 *
 * The rectangular region is specified by a top/left cell x,y and a
 * bottom/right cell x,y, inclusive.  Routine assumes top/left x is
 * even and bottom/right x is odd for cell-word alignment. This is,
 * because this routine is heavily optimized for speed, by always
 * blanking as much space as possible in one go.
 *
 * in:
 *   topx - top/left cell x position (must be even)
 *   topy - top/left cell y position
 *   botx - bottom/right cell x position (must be odd)
 *   boty - bottom/right cell y position
 */

static void blank_out(int topx, int topy, int botx, int boty)
{
    UWORD color;
    int pair, pairs, row, rows, offs;
    UBYTE *addr;

#if CONF_WITH_VIDEL
    if (TRUECOLOR_MODE) {
        blank_out16(topx, topy, botx, boty);
        return;
    }
#endif

    color = v_col_bg;                   /* bg color value */

    addr = cell_addr(topx, topy).pxaddr;   /* running pointer to screen */

    /*
     * # of cell-pairs per row in region - 1
     *
     * e.g. topx = 2, botx = 5, so pairs = 2
     */
    pairs = (botx - topx + 1) / 2;      /* pairs of characters */

    /* calculate the BYTE offset from the end of one row to next start */
    offs = v_lin_wr - pairs * 2 * v_planes;

    /*
     * # of lines in region - 1
     *
     * see comments re cell-pairs above
     */
    rows = (boty - topy + 1) * v_cel_ht;

    if (v_planes > 1) {
        /* Color modes are optimized for handling 2 planes at once */
        ULONG pair_planes[4];        /* bits on screen for 8 planes max */
        UWORD i;

        /* Precalculate the pairs of plane data */
        for (i = 0; i < v_planes / 2; i++) {
            /* set the high WORD of our LONG for the current plane */
            if (color & 0x0001)
                pair_planes[i] = 0xffff0000;
            else
                pair_planes[i] = 0x00000000;
            color >>= 1;        /* get next bit */

            /* set the low WORD of our LONG for the current plane */
            if (color & 0x0001)
                pair_planes[i] |= 0x0000ffff;
            color >>= 1;        /* get next bit */
        }

        /* do all rows in region */
        for (row = rows; row--;) {
            /* loop through all cell pairs */
            for (pair = pairs; pair--;) {
                for (i = 0; i < v_planes / 2; i++) {
                    *(ULONG*)addr = pair_planes[i];
                    addr += sizeof(ULONG);
                }
            }
            addr += offs;       /* skip non-region area with stride advance */
        }
    }
    else {
        /* Monochrome mode */
        UWORD pl;               /* bits on screen for current plane */

        /* set the WORD for plane 0 */
        if (color & 0x0001)
            pl = 0xffff;
        else
            pl = 0x0000;

        /* do all rows in region */
        for (row = rows; row--;) {
            /* loop through all cell pairs */
            for (pair = pairs; pair--;) {
                *(UWORD*)addr = pl;
                addr += sizeof(UWORD);
            }
            addr += offs;       /* skip non-region area with stride advance */
        }
    }
}



/*
 * scroll_up - Scroll upwards
 *
 *
 * Scroll copies a source region as wide as the screen to an overlapping
 * destination region on a one cell-height offset basis.  Two entry points
 * are provided:  Partial-lower scroll-up, partial-lower scroll-down.
 * Partial-lower screen operations require the cell y # indicating the
 * top line where scrolling will take place.
 *
 * After the copy is performed, any non-overlapping area of the previous
 * source region is "erased" by calling blank_out which fills the area
 * with the background color.
 *
 * in:
 *   top_line - cell y of cell line to be used as top line in scroll
 */

static void scroll_up(const CHAR_ADDR src, CHAR_ADDR dst, ULONG count)
{
    /* move BYTEs of memory*/
    memmove(dst.pxaddr, src.pxaddr, count);

    /* exit thru blank out, bottom line cell address y to top/left cell */
    blank_out(0, v_cel_my , v_cel_mx, v_cel_my);   
}



/*
 * scroll_down - Scroll (partially) downwards
 */

static void scroll_down(const CHAR_ADDR src, CHAR_ADDR dst, LONG count, UWORD start_line)
{
    /* move BYTEs of memory*/
    memmove(dst.pxaddr, src.pxaddr, count);

    /* exit thru blank out */
    blank_out(0, start_line , v_cel_mx, start_line);   
}


static void paint_cursor(void)
{
    neg_cell(v_cur_ad);
}


const CONOUT_DRIVER conout_atarifb =
{
    init,
    blank_out,
    neg_cell,
    next_cell,
    NULL, /* no blink routine */
    scroll_up,
    scroll_down,
    get_char_source,
    cell_addr,
    cell_xfer,
    paint_cursor,
    paint_cursor, /* painting/unpainting are symetric operations */
    0L /* Use default method for blinking */
};

#endif