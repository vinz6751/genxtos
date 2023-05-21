/*
 * a2560u_conout_text.c - VICKY text mode driver
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
#include "../foenix/regutils.h"

static void cursor_moved(void);


static void init(const Fonthead *font)
{
    v_cel_mx++;
    v_cel_wr = v_cel_mx;
    v_cur_ad.cellno = 0; /* First cell in text memory */
    a2560u_bios_text_init();
    cursor_moved();
}


static CHAR_ADDR cell_addr(UWORD x, UWORD y)
{
    return (CHAR_ADDR)(UWORD)(v_cel_mx * y + x + v_cur_of);
}


static int get_char_source(unsigned char c, CHAR_ADDR *src)
{
    (*src).cellno = c;
    /* If the character didn't exist, we would have a blank in the font memory */
    return -1;
}


static void cell_xfer(CHAR_ADDR src, CHAR_ADDR dst)
{    
    R8(VICKY_TEXT+dst.cellno) = src.cellno;
    
    if (v_stat_0 & M_REVID)  
        R8(VICKY_TEXT_COLOR + dst.cellno) = v_col_bg << 4 | v_col_fg;
    else
        R8(VICKY_TEXT_COLOR + dst.cellno) = v_col_fg << 4 | v_col_bg;
}


static void neg_cell(CHAR_ADDR cell)
{
    rolb(R8(VICKY_TEXT_COLOR+cell.cellno),4);
}


static void next_cell(void)
{
    v_cur_ad.cellno++;
}


static void cursor_moved(void)
{
    vicky2_set_text_cursor_xy(v_cur_cy, v_cur_cx);
}


static void blank_out(int topx, int topy, int botx, int boty)
{
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
    cell = cell_addr(topx, topy).cellno;
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
}


static void scroll_up(const CHAR_ADDR src, CHAR_ADDR dst, ULONG count)
{
    /* scroll the text memory */
    memmove((void*)(dst.cellno + VICKY_TEXT), (void*)(src.cellno + VICKY_TEXT), count);
    memmove((void*)(dst.cellno + VICKY_TEXT_COLOR), (void*)(src.cellno + VICKY_TEXT_COLOR), count);        

    /* exit thru blank out, bottom line cell address y to top/left cell */
    blank_out(0, v_cel_my , v_cel_mx, v_cel_my);
}


static void scroll_down(const CHAR_ADDR src, CHAR_ADDR dst, LONG count, UWORD start_line)
{
    /* scroll the text memory */
    memmove((void*)(dst.cellno + VICKY_TEXT), (void*)(src.cellno + VICKY_TEXT), count);
    memmove((void*)(dst.cellno + VICKY_TEXT_COLOR), (void*)(src.cellno + VICKY_TEXT_COLOR), count);

    /* exit thru blank out */
    blank_out(0, start_line , v_cel_mx, start_line);
}


static void paint_cursor(void)
{
    /* VICKY is in charge */
    vicky2_show_cursor();
}


static void unpaint_cursor(void)
{
    /* VICKY is in charge */
    vicky2_hide_cursor();
}



static void blink_cursor(void)
{
    /* Do nothing. VICKY blinks the cursor */
}

const CONOUT_DRIVER a2560u_conout_text =
{
    init,
    blank_out,
    neg_cell,
    next_cell,
    cursor_moved,
    scroll_up,
    scroll_down,
    get_char_source,
    cell_addr,
    cell_xfer,
    paint_cursor,
    unpaint_cursor,
    blink_cursor
};

#endif /* MACHINE_A2560U */ 