/*
 * a2560_conout_text.c - VICKY text mode driver
 *
 *
  * Copyright (C) 2023 The EmuTOS development team
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

//#define ENABLE_KDEBUG

#include "emutos.h"

#if defined(MACHINE_A2560U) || defined(MACHINE_A2560K) || defined(MACHINE_A2560M) || defined(MACHINE_A2560X) || defined(MACHINE_GENX)

#include "asm.h"
#include "lineavars.h"
#include "tosvars.h"            /* for v_bas_ad */
#include "string.h"
#include "conout.h"
#include "font.h"
#include "a2560_bios.h"
#include "../foenix/regutils.h"

static void cursor_moved(void);

static struct {
    uint16_t max_width;
    uint16_t max_height;
    uint16_t text_width;
    uint16_t text_height;
    uint16_t left_border_width;
} L;

static void init(const Fonthead *font)
{
#if defined(MACHINE_A2560M)
    // The configured text window is directly what we need to interpret, there
    // are no borders like on the U,X,K etc.
    uint32_t text_window_size = vicky->ctrl->text_window_size;
    L.max_width = L.text_width = (text_window_size & 0x000000ff);
    L.max_height = L.text_height = (text_window_size & 0x0000ff00) >> 8;
#else
    int border_width_chars, border_height_chars;

    if (vicky->ctrl->border_control & VICKY_BORDER_ENABLE) {
        border_width_chars = ((vicky->ctrl->border_control & VICKY_BORDER_WIDTH) >> 8) / font->max_cell_width;
        border_height_chars = ((vicky->ctrl->border_control & VICKY_BORDER_HEIGHT) >> 16) / font->form_height;
    }
    else {
        border_width_chars = border_height_chars = 0;
    }

    L.max_width = V_REZ_HZ / font->max_cell_width;
    L.max_height = V_REZ_VT / font->form_height;
    L.text_width = L.max_width - 2 * border_width_chars;
    L.text_height = L.max_height - 2 * border_height_chars;
#endif

    v_cel_mx = L.text_width - 1;
    v_cel_my = L.text_height - 1;
    v_cel_wr = L.max_width;
    a2560_debugnl("v_cel_mx=%d v_cel_my=%d v_cel_wr=%d", v_cel_mx, v_cel_my, v_cel_wr);

    v_cur_ad.cellno = 0; /* First cell in text memory */
    a2560_bios_text_init();
    cursor_moved();
}


static CHAR_ADDR cell_addr(UWORD x, UWORD y)
{
    return (CHAR_ADDR)(UWORD)((v_cel_mx + 1) * y + x + v_cur_of);
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
        vicky->text_memory->color[dst.cellno] = v_col_bg << 4 | v_col_fg;
    else
        vicky->text_memory->color[dst.cellno] = v_col_fg << 4 | v_col_bg;
}


static void neg_cell(CHAR_ADDR cell)
{
    rolb(vicky->text_memory->color[cell.cellno],4);
}


static void next_cell(void)
{
    v_cur_ad.cellno++;
}


static void cursor_moved(void)
{
    vicky2_set_text_cursor_xy(vicky, v_cur_cy, v_cur_cx);
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

    //a2560_debugnl("blank_out(%d,%d,%d,%d)",topx,topy,botx,boty);

    next_line = v_cel_wr;
    nrows = boty - topy + 1;
    ncolumns = botx - topx + 1;
    cell = cell_addr(topx, topy).cellno;
    cell_colour = v_col_fg << 4 | v_col_bg;
    line = &(vicky->text_memory->text[cell]);
    colour = &(vicky->text_memory->color[cell]);

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
#if defined(MACHINE_A2560U) || defined(MACHINE_A2560K) || defined(MACHINE_A2560M) || defined(MACHINE_A2560X) || defined(MACHINE_GENX)
    uint8_t *s,*d,*sc,*dc;

    d = &vicky->text_memory->text[dst.cellno];
    s = &vicky->text_memory->text[src.cellno];
    dc = &(vicky->text_memory->color[dst.cellno]);
    sc = &(vicky->text_memory->color[src.cellno]);

    // Unfortuntely, the X can only copy byte by byte :(
    // Unroll loop for performance as mitigation
    uint16_t cnt = count/8;
    cnt--;
    do {
        // Scroll the text memory
        *d++ = *s++;
        *d++ = *s++;
        *d++ = *s++;
        *d++ = *s++;
        *d++ = *s++;
        *d++ = *s++;
        *d++ = *s++;
        *d++ = *s++;
        // Scroll the color memory
        *dc++ = *sc++;
        *dc++ = *sc++;
        *dc++ = *sc++;
        *dc++ = *sc++;
        *dc++ = *sc++;
        *dc++ = *sc++;
        *dc++ = *sc++;
        *dc++ = *sc++;
    } while (--cnt != 0xffff); // This is supposed to produce a 'dbra'
#else
    // Scroll the text memory
    memmove((void*)(&vicky->text_memory->text[dst.cellno]), (void*)(&vicky->text_memory->text[src.cellno]), count);
    // Scroll the color memory
    memmove((void*)(&(vicky->text_memory->color[dst.cellno])), (void*)(&(vicky->text_memory->color[src.cellno])), count);
#endif

    /* exit thru blank out, bottom line cell address y to top/left cell */
    blank_out(0, v_cel_my , v_cel_mx, v_cel_my);
}


static void scroll_down(const CHAR_ADDR src, CHAR_ADDR dst, LONG count, UWORD start_line)
{
    /* scroll the text memory */
    memmove((void*)(&vicky->text_memory->text[dst.cellno]), (void*)(&vicky->text_memory->text[src.cellno]), count);
    memmove((void*)(&(vicky->text_memory->color[dst.cellno])), (void*)(&(vicky->text_memory->color[src.cellno])), count);

    /* exit thru blank out */
    blank_out(0, start_line , v_cel_mx, start_line);
}


static void paint_cursor(void)
{
    /* VICKY is in charge */
    vicky2_show_cursor(vicky);
}


static void unpaint_cursor(void)
{
    /* VICKY is in charge */
    vicky2_hide_cursor(vicky);
}


static void blink_cursor(void)
{
    /* Do nothing. VICKY blinks the cursor */
}

const CONOUT_DRIVER a2560_conout_text =
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

#endif
