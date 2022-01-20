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

#define ENABLE_KDEBUG

#include "emutos.h"
#include "asm.h"
#include "lineavars.h"
#include "tosvars.h"            /* for v_bas_ad */
#include "sound.h"              /* for bell() */
#include "string.h"
#include "conout.h"
#include "font.h"
#include "a2560u.h"



#define PLANE_OFFSET    2       /* interleaved planes */


/*
 * cell_addr - convert cell X,Y to a screen address.
 *
 *
 * convert cell X,Y to a screen address. also clip cartesian coordinates
 * to the limits of the current screen.
 */

CHAR_ADDR cell_addr(UWORD x, UWORD y)
{
    /* check bounds against screen limits */
    if (x > v_cel_mx)
        x = v_cel_mx;           /* clipped x */

    if (y > v_cel_my)
        y = v_cel_my;           /* clipped y */

#ifdef MACHINE_A2560U
# if CONF_WITH_A2560U_TEXT_MODE
    return v_cel_mx * y + x + v_cur_of;
# else
    return v_bas_ad + (ULONG)v_cel_wr * y + x * 8 + v_cur_of;
# endif
#else
    ULONG disx, disy;

    /*
     * v_planes cannot be more than 8, so as long as there are no more
     * than 4000 characters per line, the result will fit in a word ...
     *
     * X displacement = even(X) * v_planes + Xmod2
     */
    disx = v_planes * (x & ~1);
    if (IS_ODD(x)) {            /* Xmod2 = 0 ? */
        disx++;                 /* Xmod2 = 1 */
    }

    /* Y displacement = Y // cell conversion factor */
    disy = (ULONG)v_cel_wr * y;

    /*
     * cell address = screen base address + Y displacement
     * + X displacement + offset from screen-begin (fix)
     */
    return v_bas_ad + disy + disx + v_cur_of;
#endif    
}



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
#if CONF_WITH_A2560U_TEXT_MODE
    R8(VICKY_TEXT+dst) = src;
    
    if (v_stat_0 & M_REVID)  
        R8(VICKY_TEXT_COLOR + dst) = v_col_bg << 4 | v_col_fg;
    else
        R8(VICKY_TEXT_COLOR + dst) = v_col_fg << 4 | v_col_bg;
#else
    UWORD fg;
    UWORD bg;

    /* check for reversed foreground and background colors */
    if (v_stat_0 & M_REVID) {
        fg = v_col_bg;
        bg = v_col_fg;
    }
    else {
        fg = v_col_fg;
        bg = v_col_bg;
    }

# if CONF_WITH_CHUNKY8
    int i,j; /* Source bitshift */
    UBYTE bsrc;
    UBYTE *c = dst;

    for (j = 0; j < v_cel_ht; j++)
    {
        bsrc = *src;
        for (i = 0; i < 8/*font width*/; i++)
        {                
            dst[i] = bsrc & 0x80 ? fg : bg;
            bsrc <<= 1;
        }
        dst += v_lin_wr;
        src += v_fnt_wr;        
    }
#  ifdef MACHINE_A2560U
    a2560u_mark_cell_dirty(c);
#  endif
# else
    UBYTE * src_sav, * dst_sav;
    int fnt_wr, line_wr;
    int plane;

    fnt_wr = v_fnt_wr;
    line_wr = v_lin_wr;

    src_sav = src;
    dst_sav = dst;

    for (plane = v_planes; plane--; ) {
        int i;

        src = src_sav;                  /* reload src */
        dst = dst_sav;                  /* reload dst */

        if (bg & 0x0001) {
            if (fg & 0x0001) {
                /* back:1  fore:1  =>  all ones */
                for (i = v_cel_ht; i--; ) {
                    *dst = 0xff;                /* inject a block */
                    dst += line_wr;
                }
            }
            else {
                /* back:1  fore:0  =>  invert block */
                for (i = v_cel_ht; i--; ) {
                    /* inject the inverted source block */
                    *dst = ~*src;
                    dst += line_wr;
                    src += fnt_wr;
                }
            }
        }
        else {
            if (fg & 0x0001) {
                /* back:0  fore:1  =>  direct substitution */
                for (i = v_cel_ht; i--; ) {
                    *dst = *src;
                    dst += line_wr;
                    src += fnt_wr;
                }
            }
            else {
                /* back:0  fore:0  =>  all zeros */
                for (i = v_cel_ht; i--; ) {
                    *dst = 0x00;                /* inject a block */
                    dst += line_wr;
                }
            }
        }

        bg >>= 1;                       /* next background color bit */
        fg >>= 1;                       /* next foreground color bit */
        dst_sav += PLANE_OFFSET;        /* top of block in next plane */
    }
# endif
#endif
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

#if CONF_WITH_A2560U_TEXT_MODE
    rolb(R8(VICKY_TEXT_COLOR+cell),4);
#else
# if CONF_WITH_CHUNKY8
    int i; /* Source bitshift */
    const int inc = v_lin_wr-4;
    UBYTE *c = cell;
    for (i = 0; i < v_cel_ht; i++)
    {
        /* TODO it seems that cursor blinking and other things involving reading from video ram don't work (yet).
         * When we read from video ram, we get 0. So the cursor is always displayed. As I got from the forum,
         * it's a known problem but I am not sure if it will be resolved. Once VDMA is implemented in the A2560U,
         * we can use it to transfer video ram to ram and do our thing. */
        /* We process 4 bytes at a time and loop is unrolled for performance */
#  if 0
        *((LONG*)cell) = ~*((LONG*)cell);
        cell += 4;
        *((LONG*)cell) = ~*((LONG*)cell);
        cell += inc;
#  else
        int j;
        for(j=7; j--;)
            cell[j] = ~cell[j];
        cell += v_lin_wr;
#  endif
    }
    a2560u_mark_cell_dirty(c);
# else
    int len;
    int plane;
    int lin_wr = v_lin_wr;
    int cell_len = v_cel_ht;

    for (plane = v_planes; plane--; ) {
        UBYTE * addr = cell;            /* top of current dest plane */

        /* reset cell length counter */
        for (len = cell_len; len--; ) {
            *addr = ~*addr;
            addr += lin_wr;
        }
        cell += PLANE_OFFSET;           /* a1 -> top of block in next plane */

    }
# endif
#endif
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

#if CONF_WITH_A2560U_TEXT_MODE
    v_cur_ad += 1;
#elif CONF_WITH_CHUNKY8
    /* Would be v_cel_with it it existed, or font->max_cell_width,
     * but the console stuff only handles 8-pixel-wide fonts */
    v_cur_ad += 8;
#else
    /* if X is even, move to next word in the plane */
    if (IS_ODD(v_cur_cx)) {
        /* x is odd */
        v_cur_ad += 1;                  /* a1 -> new cell */
        return 0;                       /* indicate no wrap needed */
    }

    /* new cell (1st plane), added offset to next word in plane */
    v_cur_ad += (v_planes << 1) - 1;
#endif

    return 0;                           /* indicate no wrap needed */
}



/*
 * invert_cell - negates the cells bits
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

void invert_cell(int x, int y)
{
    /* fetch x and y coords and invert cursor. */
    neg_cell(cell_addr(x, y));
}



/*
 * move_cursor - move the cursor.
 *
 * move the cursor and update global parameters
 * erase the old cursor (if necessary) and draw new cursor (if necessary)
 *
 * in:
 * d0.w    new cell X coordinate
 * d1.w    new cell Y coordinate
 */

void move_cursor(int x, int y)
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
#ifdef MACHINE_A2560U
    a2560u_update_cursor();
#endif

    if (v_stat_0 & M_CVIS) {
        /* is cursor flashing? */
        if (v_stat_0 & M_CFLASH) {
            v_stat_0 &= ~M_CVIS;  /* yes, make invisible...semaphore. */

            /* if cursor flashing enabled and cursor is presently painted, unpaint */
            if (v_stat_0 & M_CSTATE)
                con_unpaint_cursor();

            /* set new coordinates and paint there */
            v_cur_ad = cell_addr(x, y);
            con_paint_cursor();

            v_stat_0 |= M_CVIS;                 /* end of critical section. */
        }
    }
    else {
        /* not visible */
        v_cur_ad = cell_addr(x, y);             /* just set new coordinates */
    }  
}



/*
 * ascii_out - prints an ascii character on the screen
 *
 * in:
 *
 * ch.w      ascii code for character
 */

void ascii_out(int ch)
{
    CHAR_ADDR src;
    CHAR_ADDR dst;
    BOOL visible;                       /* was the cursor visible? */

#if CONF_WITH_A2560U_TEXT_MODE
    src = ch;
#else
    src = char_addr(ch);                /* a0 -> get character source */
    if (src == NULL)
        return;                         /* no valid character */
#endif

    dst = v_cur_ad;                     /* a1 -> get destination */

    visible = v_stat_0 & M_CVIS;        /* test visibility bit */
    if (visible) {
        v_stat_0 &= ~M_CVIS;            /* start of critical section */
    }

    /* put the cell out (this covers the cursor) */
    cell_xfer(src, dst);

    /* advance the cursor and update cursor address and coordinates */
    if (next_cell()) {
        /* CRLF */
        CHAR_ADDR cell;
        UWORD y = v_cur_cy;

        /* perform cell carriage return. */
#if CONF_WITH_A2560U_TEXT_MODE
        cell = VICKY_TEXT + v_cel_mx * y;
#else       
        cell = v_bas_ad + (ULONG)v_cel_wr * y;
#endif        
        v_cur_cx = 0;                   /* set X to first cell in line */

        /* perform cell line feed. */
        if (y < v_cel_my) {
            cell += v_cel_wr;           /* move down one cell */
            v_cur_cy = y + 1;           /* update cursor's y coordinate */
        }
        else {
            scroll_up(0);               /* scroll from top of screen */
        }
        v_cur_ad = cell;                /* update cursor address */   
    }

#ifdef MACHINE_A2560U
    a2560u_update_cursor();
#endif

    /* if visible */
    if (visible) {
        con_paint_cursor();             /* display cursor. */
        v_stat_0 |= M_CVIS;             /* end of critical section. */
    }
}



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

void blank_out(int topx, int topy, int botx, int boty)
{
#if CONF_WITH_A2560U_TEXT_MODE
    UWORD next_line;    
    int nrows,ncolumns;
    int row,column;
    uint8_t *line;
    uint8_t *colour;
    uint16_t cell;
    uint8_t cell_colour;
    
    next_line = v_cel_mx;
    nrows = boty - topy + 1;
    ncolumns = botx - topx + 1;
    cell = (uint16_t)cell_addr(topx, topy);
    cell_colour = v_col_fg << 4 | v_col_bg;
    line = (uint8_t*)(VICKY_TEXT + cell);
    colour = (uint8_t*)(VICKY_TEXT_COLOR + cell);

    /* TODO: could do with optimisation */
    for (row = 0; row < nrows; row++)    
    {
        for (column = 0; column < ncolumns; column++)
        {
            line[column] = ' ';
            colour[column] = cell_colour;
        }
        line += next_line;
        colour += next_line;
    }
#else
    UWORD color = v_col_bg;             /* bg color value */
    int pair, pairs, row, rows, offs;
    UBYTE * addr = cell_addr(topx, topy);   /* running pointer to screen */

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

# ifdef MACHINE_A2560U
    a2560u_mark_screen_dirty();
# endif
#endif
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

void scroll_up(UWORD top_line)
{
    ULONG count;
    CHAR_ADDR src;
    CHAR_ADDR dst;

#if CONF_WITH_A2560U_TEXT_MODE
    dst = cell_addr(0, top_line);
    src = cell_addr(0, top_line + 1);
    count = (ULONG)v_cel_mx * (v_cel_my - top_line);
#else
    /* screen base addr + cell y nbr * cell wrap */
    dst = v_bas_ad + (ULONG)top_line * v_cel_wr;

    /* form source address from cell wrap + base address */
    src = dst + v_cel_wr;

    /* form # of bytes to move */
    count = (ULONG)v_cel_wr * (v_cel_my - top_line);
#endif

    /* move BYTEs of memory*/
#if CONF_WITH_A2560U_TEXT_MODE    
    memmove((void*)(dst + VICKY_TEXT), (void*)(src + VICKY_TEXT), count);
    memmove((void*)(dst + VICKY_TEXT_COLOR), (void*)(src + VICKY_TEXT_COLOR), count);
#else
    memmove(dst, src, count);
#endif

    /* exit thru blank out, bottom line cell address y to top/left cell */
    blank_out(0, v_cel_my , v_cel_mx, v_cel_my);

#if defined(MACHINE_A2560U) && !CONF_WITH_A2560U_TEXT_MODE
    // already called by blank_out
    //a2560u_mark_screen_dirty();
#endif    
}



/*
 * scroll_down - Scroll (partially) downwards
 */

void scroll_down(UWORD start_line)
{
    ULONG count;
    CHAR_ADDR src;
    CHAR_ADDR dst;

#if CONF_WITH_A2560U_TEXT_MODE
    src = cell_addr(0, start_line);
    dst = cell_addr(0, start_line + 1);
    count = (ULONG)v_cel_mx * (v_cel_my - start_line);
#else
    /* screen base addr + offset of start line */
    src = v_bas_ad + (ULONG)start_line * v_cel_wr;

    /* form destination from source + cell wrap */
    dst = src + v_cel_wr;

    /* form # of bytes to move */
    count = (ULONG)v_cel_wr * (v_cel_my - start_line);
#endif

    /* move BYTEs of memory*/
#if CONF_WITH_A2560U_TEXT_MODE
    memmove((void*)(dst + VICKY_TEXT), (void*)(src + VICKY_TEXT), count);
    memmove((void*)(dst + VICKY_TEXT_COLOR), (void*)(src + VICKY_TEXT_COLOR), count);
#else    
    memmove(dst, src, count);
#endif

    /* exit thru blank out */
    blank_out(0, start_line , v_cel_mx, start_line);

#if defined(MACHINE_A2560U) && !CONF_WITH_A2560U_TEXT_MODE
    // already called by blank_out
    //a2560u_mark_screen_dirty();
#endif   
}


void con_paint_cursor(void)
{
    v_stat_0 |= M_CSTATE;
    
#if CONF_WITH_A2560U_TEXT_MODE
    /* VICKY is in charge */
    vicky2_show_cursor();
#else
    neg_cell(v_cur_ad);
#endif
    v_cur_tim = v_period; /* reset the timer for blinking */
}


void con_unpaint_cursor(void)
{
    v_stat_0 &= ~M_CSTATE;

#if CONF_WITH_A2560U_TEXT_MODE
    /* VICKY is in charge */
    vicky2_hide_cursor();
#else
    neg_cell(v_cur_ad);
#endif    
}