/*
 * a2560u_conout_text.c - VICKY framebuffer mode driver
 *
 *
  * Copyright (C) 2022 The EmuTOS development team
 *
 * Authors:
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

#ifdef MACHINE_A2560U

#include "asm.h"
#include "lineavars.h"
#include "tosvars.h"            /* for v_bas_ad */
#include "string.h"
#include "conout.h"
#include "font.h"
#include "a2560u_bios.h"
#include "../foenix/shadow_fb.h"
#include "../foenix/regutils.h"


static void init(const Fonthead *font)
{
    KDEBUG(("conout_bmp->init\n"));
    v_cel_wr = v_lin_wr * v_cel_ht;
    v_cur_ad.pxaddr = v_bas_ad;

    /* Stop text mode and start bitmap */
    R32(VICKY_CTRL) &= ~VICKY_A_CTRL_TEXT; 
    R32(VICKY_CTRL) = VICKY_A_CTRL_GFX|VICKY_A_CTRL_BITMAP;
    KDEBUG(("conout_bmp->init done\n"));
}


static CHAR_ADDR cell_addr(UWORD x, UWORD y)
{
    return (CHAR_ADDR)(v_bas_ad + (ULONG)v_cel_wr * y + x * 8 + v_cur_of);
}


static int get_char_source(unsigned char c, CHAR_ADDR *src)
{
    (*src).pxaddr = char_addr(c);   /* a0 -> get character source */
    return src != NULL;             /* return false if no valid character */
}


static void cell_xfer(CHAR_ADDR src, CHAR_ADDR dst)
{    
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

    int j; /* line of the cell */
    UBYTE bsrc;
    UBYTE *d = dst.pxaddr;
    /* precompute so we don't have to do that in the loop */
    const UWORD v_lin_wr_advance = v_lin_wr - 8;
    UWORD bgbg;

    /* We take the bet at least one line will be blank */
    ((UBYTE*)&bgbg)[0] = bg;
    ((UBYTE*)&bgbg)[1] = bg;

    for (j = v_cel_ht; --j>=0 ; )
    {
        if ((bsrc = *(src.pxaddr))) {
        
#if 0
        int i;
        for (i = 0; i < 8/*font width*/; i++)
        {                
            dst.pxaddr[i] = bsrc & 0x80 ? fg : bg;
            bsrc <<= 1;
        }
#elif 1
        /* Faster version, for fixed 8-pixel font */
        *d++ = bsrc & 0x80 ? fg : bg;
        *d++ = bsrc & 0x40 ? fg : bg;
        *d++ = bsrc & 0x20 ? fg : bg;
        *d++ = bsrc & 0x10 ? fg : bg;
        *d++ = bsrc & 0x08 ? fg : bg;
        *d++ = bsrc & 0x04 ? fg : bg;
        *d++ = bsrc & 0x02 ? fg : bg;
        *d++ = bsrc & 0x01 ? fg : bg;
        d += v_lin_wr_advance;
#endif
        }
        else {
            UWORD *e = (UWORD*)d;
            /* Optimise for white space */
            *e++ = bgbg;
            *e++ = bgbg;
            *e++ = bgbg;
            *e++ = bgbg;
            d += v_lin_wr;
        }

        src.pxaddr += v_fnt_wr;
    }

#if CONF_WITH_A2560U_SHADOW_FRAMEBUFFER
    a2560u_sfb_mark_cell_dirty(dst.pxaddr);
#endif
}


static void neg_cell(CHAR_ADDR cell)
{
    const int inc = v_lin_wr - 3 * sizeof(UWORD);
    int i;
#if CONF_WITH_A2560U_SHADOW_FRAMEBUFFER
    UBYTE *c = cell.pxaddr;
#endif

    for (i = 0; i < v_cel_ht; i++)
    {
        /* We process 2 bytes at a time and loop to render 8 pixels (width of the font), is unrolled for performance */
        *((UWORD*)cell.pxaddr) = ~*((UWORD*)cell.pxaddr);
        cell.pxaddr += sizeof(UWORD);
        *((UWORD*)cell.pxaddr) = ~*((UWORD*)cell.pxaddr);
        cell.pxaddr += sizeof(UWORD);
        *((UWORD*)cell.pxaddr) = ~*((UWORD*)cell.pxaddr);
        cell.pxaddr += sizeof(UWORD);
        *((UWORD*)cell.pxaddr) = ~*((UWORD*)cell.pxaddr);
        cell.pxaddr += inc;
    }

#if CONF_WITH_A2560U_SHADOW_FRAMEBUFFER
    a2560u_sfb_mark_cell_dirty(c);
#endif
}


static void next_cell(void)
{
    v_cur_ad.pxaddr += 8;  /* Would be v_cel_with it it existed, or font->max_cell_width, but the console stuff only handles 8-pixel-wide fonts */
}


static void blank_out(int topx, int topy, int botx, int boty)
{
    UWORD color = v_col_bg;             /* bg color value */
    int pair, pairs, row, rows, offs;
    UBYTE * addr = cell_addr(topx, topy).pxaddr;   /* running pointer to screen */

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

#if CONF_WITH_A2560U_SHADOW_FRAMEBUFFER
    a2560u_sfb_mark_screen_dirty();
#endif
}


static void scroll_up(const CHAR_ADDR src, CHAR_ADDR dst, ULONG count)
{
    /* move BYTEs of memory*/
    memmove(dst.pxaddr, src.pxaddr, count);

    /* exit thru blank out, bottom line cell address y to top/left cell */
    blank_out(0, v_cel_my , v_cel_mx, v_cel_my);
}


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


static void unpaint_cursor(void)
{
    // FIXME if you remove this, when doing cr/lf the cursor may not be unpainted from the original line
    // I have no idea why ! It may be time related, or something to do with the shadow frame buffer copy
    // to VRAM during the VBL.
    long i;
    volatile int truc;
    for (i=1; i<5; i++)
        truc++;

    neg_cell(v_cur_ad);
}


const CONOUT_DRIVER a2560u_conout_bmp =
{
    init,
    blank_out,
    neg_cell,
    next_cell,
    0L,
    scroll_up,
    scroll_down,
    get_char_source,
    cell_addr,
    cell_xfer,
    paint_cursor,
    unpaint_cursor,
    0L, /* Use default method for blinking */
};

#endif /* MACHINE_A2560U */