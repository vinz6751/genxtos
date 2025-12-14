/*
 * a2560_conout_bmp.c - VICKY framebuffer mode driver
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

#if defined(MACHINE_A2560U) || defined(MACHINE_A2560K) || defined(MACHINE_A2560M) || defined(MACHINE_A2560X) || defined(MACHINE_GENX)

#include "asm.h"
#include "lineavars.h"
#include "tosvars.h"            /* for v_bas_ad */
#include "string.h"
#include "conout.h"
#include "font.h"
#include "a2560_bios.h"
#include "../foenix/shadow_fb.h"
#include "../foenix/regutils.h"


static void init(const Fonthead *font)
{
    KDEBUG(("conout_bmp->init\n"));
    v_cel_mx = (V_REZ_HZ / font->max_cell_width) - 1;
    v_cel_my = (V_REZ_VT / font->form_height) - 1;
    v_cel_wr = v_lin_wr * v_cel_ht;
    v_cur_ad.pxaddr = v_bas_ad;

    /* Stop text mode and start bitmap */
    vicky->ctrl->control &= ~VICKY_CTRL_TEXT;
    vicky->ctrl->control = VICKY_CTRL_GFX|VICKY_CTRL_BITMAP;
    KDEBUG(("conout_bmp->init done\n"));
}


#if CONF_WITH_A2560U_SHADOW_FRAMEBUFFER
static UBYTE* charaddr_to_vram_address(CHAR_ADDR c)
{
  return (UBYTE*)((ULONG)c.pxaddr. - (ULONG)v_bas_ad + (ULONG)a2560_bios_vram_fb);
}
#endif


static __inline__ CHARADDR cell_offset(UWORD x, UWORD y)
{
  return (CHAR_ADDR)((ULONG)v_cel_wr * y + x * 8 + v_cur_of);
}


static CHAR_ADDR cell_addr(UWORD x, UWORD y)
{
  return (CHAR_ADDR)(v_bas_ad + cell_offset(x, y));
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
	UWORD font_width;
	font_width = 8;

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
    const UWORD v_lin_wr_advance = v_lin_wr - font_width;
    UWORD bgbg;

    /* We take the bet at least one line will be blank */
    ((UBYTE*)&bgbg)[0] = bg;
    ((UBYTE*)&bgbg)[1] = bg;

#if CONF_WITH_A2560U_SHADOW_FRAMEBUFFER
    /* Calculate the offset in VRAM */
    UBYTE *vram_dst =  charaddr_to_vram_address(dst);
#endif
    
    for (j = v_cel_ht; --j>=0 ; )
    {
	    if ((bsrc = *(src.pxaddr))) {
        
        int i;
        for (i = 0; i < font_width; i++)
        {                
            *d = bsrc & 0x80 ? fg : bg;
#if CONF_WITH_A2560U_SHADOW_FRAMEBUFFER
	        *vram_dst++ = *d++;
#endif
            bsrc <<= 1;
        }
        else {
            UWORD *e = (UWORD*)d;
            /* Optimise for white space */
            *e++ = bgbg;
            *e++ = bgbg;
            *e++ = bgbg;
            *e++ = bgbg;
#if CONF_WITH_A2560U_SHADOW_FRAMEBUFFER
	        *vram_dst++ = bgbg;
			*vram_dst++ = bgbg;
			*vram_dst++ = bgbg;
			*vram_dst++ = bgbg;
#endif
            d += v_lin_wr;
        }

        src.pxaddr += v_fnt_wr;
    }
}


static void neg_cell(CHAR_ADDR cell)
{
    const int inc = v_lin_wr - 3 * sizeof(UWORD);
    int i,j;
	UWORD *d = (UWORD*)cell.pxaddr;
#if CONF_WITH_A2560U_SHADOW_FRAMEBUFFER
    /* Calculate the offset in VRAM */
    UWORD *vram_dst =  (UWORD*)charaddr_to_vram_address(cell);
#endif

    for (i = 0; i < v_cel_ht; i++)
    {
	  for (j=0; j<4; j++)
		{
		  /* We process 2 bytes (=pixels) at a time */
		  d[j] = ~d[j];
#if CONF_WITH_A2560U_SHADOW_FRAMEBUFFER
		  vram_dst[j] = d[j];
#endif
		}
        d += v_lin_wr;
#if CONF_WITH_A2560U_SHADOW_FRAMEBUFFER
		vram_dst += v_lin_wr;
#endif
    }
}


static void next_cell(void)
{
    v_cur_ad.pxaddr += 8;  /* Would be v_cel_with it it existed, or font->max_cell_width, but the console stuff only handles 8-pixel-wide fonts */
}


static void blank_out(int topx, int topy, int botx, int boty)
{
    UWORD color = v_col_bg;             /* bg color value */
    int pair, pairs, row, rows, offs;
	CHARADDR first_cell = cell_addr(topx, topy);
    UBYTE * addr = first_cell.pxaddr;   /* running pointer to screen */
#if CONF_WITH_A2560U_SHADOW_FRAMEBUFFER
    /* Calculate the offset in VRAM */
    UBYTE *vram_dst =  charaddr_to_vram_address(first_cell);
#endif

	
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
#if CONF_WITH_A2560U_SHADOW_FRAMEBUFFER
				/* We can only write words, not longs :( */
				*vram_dst = ((UWORD*)(&(pair_planes[i])))[0];
				*vram_dst = ((UWORD*)(&(pair_planes[i])))[1];
				vram_dst += sizeof(ULONG);
#endif
            }
        }
        addr += offs;       /* skip non-region area with stride advance */
#if CONF_WITH_A2560U_SHADOW_FRAMEBUFFER
		vram_dst += offs;
#endif		
    }
}


static void scroll_up(const CHAR_ADDR src, CHAR_ADDR dst, ULONG count)
{
    /* move BYTEs of memory*/
    memmove(dst.pxaddr, src.pxaddr, count);
#if CONF_WITH_A2560U_SHADOW_FRAMEBUFFER
    memmove(charaddr_to_vram_address(dst), charaddr_to_vram_address(src), count);
#endif

    /* exit thru blank out, bottom line cell address y to top/left cell */
    blank_out(0, v_cel_my , v_cel_mx, v_cel_my);

}


static void scroll_down(const CHAR_ADDR src, CHAR_ADDR dst, LONG count, UWORD start_line)
{
    /* move BYTEs of memory*/
    memmove(dst.pxaddr, src.pxaddr, count);
#if CONF_WITH_A2560U_SHADOW_FRAMEBUFFER
    memmove(charaddr_to_vram_address(dst), charaddr_to_vram_address(src), count);
#endif
	
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


const CONOUT_DRIVER a2560_conout_bmp =
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
