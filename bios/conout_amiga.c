/*
 * conout_amiga.c - console text output for Amiga planar framebuffers
 *
 * Amiga bitplanes are separate contiguous buffers, not Atari interleaved
 * words.  This driver is the planar counterpart of conout_atarifb.c.
 *
 * Copyright (C) 2026 The EmuTOS development team
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

/* #define ENABLE_KDEBUG */

#include "emutos.h"

#ifdef MACHINE_AMIGA

#include "asm.h"
#include "lineavars.h"
#include "tosvars.h"
#include "string.h"
#include "conout.h"
#include "font.h"
#include "amiga.h"

static void init(const Fonthead *font)
{
    v_cel_mx = (V_REZ_HZ / font->max_cell_width) - 1;
    v_cel_my = (V_REZ_VT / font->form_height) - 1;
    v_cel_wr = v_lin_wr * v_cel_ht;
    v_cur_ad.pxaddr = v_bas_ad;
}

/*
 * cell_addr - address of the character cell in plane 0
 *
 * On Amiga planar screens each cell is one byte wide per plane; the next
 * character is simply the next byte in the plane.
 */
static CHAR_ADDR cell_addr(UWORD x, UWORD y)
{
    if (x > v_cel_mx)
        x = v_cel_mx;
    if (y > v_cel_my)
        y = v_cel_my;

    return (CHAR_ADDR)(v_bas_ad + (ULONG)v_cel_wr * y + x + v_cur_of);
}

static int get_char_source(unsigned char c, CHAR_ADDR *src)
{
    (*src).pxaddr = char_addr(c);
    return src != NULL;
}

static void cell_xfer(CHAR_ADDR src, CHAR_ADDR dst)
{
    UBYTE *src_sav = src.pxaddr;
    ULONG cell_off = (ULONG)(dst.pxaddr - v_bas_ad);
    UWORD fg, bg;
    int fnt_wr = v_fnt_wr;
    int line_wr = v_lin_wr;
    int plane;

    if (v_stat_0 & M_REVID) {
        fg = v_col_bg;
        bg = v_col_fg;
    } else {
        fg = v_col_fg;
        bg = v_col_bg;
    }

    for (plane = 0; plane < v_planes; plane++) {
        UBYTE *d = amiga_plane_base(plane) + cell_off;
        UBYTE *s = src_sav;
        int i;

        if (bg & 0x0001) {
            if (fg & 0x0001) {
                for (i = v_cel_ht; i--; ) {
                    *d = 0xff;
                    d += line_wr;
                }
            } else {
                for (i = v_cel_ht; i--; ) {
                    *d = ~*s;
                    d += line_wr;
                    s += fnt_wr;
                }
            }
        } else {
            if (fg & 0x0001) {
                for (i = v_cel_ht; i--; ) {
                    *d = *s;
                    d += line_wr;
                    s += fnt_wr;
                }
            } else {
                for (i = v_cel_ht; i--; ) {
                    *d = 0x00;
                    d += line_wr;
                }
            }
        }

        bg >>= 1;
        fg >>= 1;
    }
}

static void neg_cell(CHAR_ADDR cell)
{
    ULONG cell_off = (ULONG)(cell.pxaddr - v_bas_ad);
    int plane, len;
    int lin_wr = v_lin_wr;
    int cell_len = v_cel_ht;

    for (plane = 0; plane < v_planes; plane++) {
        UBYTE *addr = amiga_plane_base(plane) + cell_off;

        for (len = cell_len; len--; ) {
            *addr = ~*addr;
            addr += lin_wr;
        }
    }
}

static void next_cell(void)
{
    v_cur_ad.pxaddr += 1;
}

static void blank_out(int topx, int topy, int botx, int boty)
{
    UWORD color = v_col_bg;
    int pairs = (botx - topx + 1) / 2;
    int rows = (boty - topy + 1) * v_cel_ht;
    int plane;
    ULONG cell_off = (ULONG)(cell_addr(topx, topy).pxaddr - v_bas_ad);

    for (plane = 0; plane < v_planes; plane++) {
        UWORD pl = (color & 0x0001) ? 0xffff : 0x0000;
        UBYTE *addr = amiga_plane_base(plane) + cell_off;
        int offs = v_lin_wr - pairs * 2;
        int row, pair;

        for (row = rows; row--; ) {
            for (pair = pairs; pair--; ) {
                *(UWORD *)addr = pl;
                addr += sizeof(UWORD);
            }
            addr += offs;
        }
        color >>= 1;
    }
}

static void scroll_up(const CHAR_ADDR src, CHAR_ADDR dst, ULONG count)
{
    ULONG src_off = (ULONG)(src.pxaddr - v_bas_ad);
    ULONG dst_off = (ULONG)(dst.pxaddr - v_bas_ad);
    int plane;

    /* count is bytes within one plane (from conout_scroll_up via v_cel_wr) */
    for (plane = 0; plane < v_planes; plane++) {
        memmove(amiga_plane_base(plane) + dst_off,
                amiga_plane_base(plane) + src_off,
                count);
    }

    blank_out(0, v_cel_my, v_cel_mx, v_cel_my);
}

static void scroll_down(const CHAR_ADDR src, CHAR_ADDR dst, LONG count, UWORD start_line)
{
    ULONG src_off = (ULONG)(src.pxaddr - v_bas_ad);
    ULONG dst_off = (ULONG)(dst.pxaddr - v_bas_ad);
    int plane;

    for (plane = 0; plane < v_planes; plane++) {
        memmove(amiga_plane_base(plane) + dst_off,
                amiga_plane_base(plane) + src_off,
                count);
    }

    blank_out(0, start_line, v_cel_mx, start_line);
}

static void paint_cursor(void)
{
    neg_cell(v_cur_ad);
}

const CONOUT_DRIVER conout_amiga =
{
    init,
    blank_out,
    neg_cell,
    next_cell,
    NULL,
    scroll_up,
    scroll_down,
    get_char_source,
    cell_addr,
    cell_xfer,
    paint_cursor,
    paint_cursor,
    0L
};

#endif /* MACHINE_AMIGA */
