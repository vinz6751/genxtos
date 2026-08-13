/*
 * raster_driver_amiga_bitplanes.c - VDI raster driver for Amiga planar FB
 *
 * Each bitplane is a separate contiguous buffer (Denise BPLxPT), unlike
 * Atari interleaved bitplanes.
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
#include "intmath.h"
#include "string.h"
#include "tosvars.h"
#include "lineavars.h"
#include "vdi_defs.h"
#include "vdistub.h"
#include "vdi_blit.h"
#include "vdi_textblit.h"
#include "raster_driver_amiga_bitplanes.h"
#include "vdi_raster_driver.h"

/*
 * setup_forms for Amiga planar screen + Atari-style memory MFDBs
 *
 * Screen: nxwd=2, nxln=v_lin_wr, nxpl=amiga_plane_size
 * Memory MFDB: keep Atari interleaved layout (nxpl=2)
 */
static BOOL amiga_setup_forms(struct blit_frame *info, const MFDB *src, const MFDB *dst)
{
    if (src->fd_addr) {
        info->s_form = src->fd_addr;
        info->s_nxwd = src->fd_nplanes * 2;
        info->s_nxln = src->fd_wdwidth * info->s_nxwd;
        info->s_nxpl = 2;
    } else {
        info->s_form = (UWORD *)v_bas_ad;
        info->s_nxwd = 2;
        info->s_nxln = v_lin_wr;
        info->s_nxpl = (WORD)amiga_plane_size;
    }

    if (dst->fd_addr) {
        info->d_form = dst->fd_addr;
        info->plane_ct = dst->fd_nplanes;
        info->d_nxwd = dst->fd_nplanes * 2;
        info->d_nxln = dst->fd_wdwidth * info->d_nxwd;
        info->d_nxpl = 2;
    } else {
        info->d_form = (UWORD *)v_bas_ad;
        info->plane_ct = v_planes;
        info->d_nxwd = 2;
        info->d_nxln = v_lin_wr;
        info->d_nxpl = (WORD)amiga_plane_size;
    }

    return (info->plane_ct > 8) ? TRUE : FALSE;
}

void amiga_swblit_rect(const VwkAttrib *attr, const Rect *rect)
{
    UWORD leftmask = 0xffff >> (rect->x1 & 0x0f);
    UWORD rightmask = 0xffff << (15 - (rect->x2 & 0x0f));
    WORD width = (rect->x2 >> 4) - (rect->x1 >> 4) + 1;
    WORD centre;
    WORD y, plane;
    UWORD color = attr->color;
    const UWORD patmsk = attr->patmsk;

    if (width == 1) {
        leftmask &= rightmask;
        rightmask = 0;
    }
    centre = width - 2 - 1;   /* -1 because the centre loops use n >= 0 */

    for (y = rect->y1; y <= rect->y2; y++) {
        int patind = patmsk & y;
        UWORD pattern = attr->patptr ? attr->patptr[patind] : 0xffff;
        UWORD c = color;

        for (plane = 0; plane < v_planes; plane++, c >>= 1) {
            UWORD *work = amiga_plane_word(plane, rect->x1, y);
            UWORD bit = c & 1;
            WORD n;

            switch (attr->wrt_mode) {
            case WM_REPLACE:
            default:
                if (bit) {
                    *work = (*work & ~leftmask) | (pattern & leftmask);
                    work++;
                    for (n = centre; n >= 0; n--)
                        *work++ = pattern;
                    if (rightmask)
                        *work = (*work & ~rightmask) | (pattern & rightmask);
                } else {
                    *work &= ~leftmask;
                    work++;
                    for (n = centre; n >= 0; n--)
                        *work++ = 0;
                    if (rightmask)
                        *work &= ~rightmask;
                }
                break;
            case WM_TRANS:
                if (bit) {
                    *work |= pattern & leftmask;
                    work++;
                    for (n = centre; n >= 0; n--)
                        *work++ |= pattern;
                    if (rightmask)
                        *work |= pattern & rightmask;
                } else {
                    *work &= ~(pattern & leftmask);
                    work++;
                    for (n = centre; n >= 0; n--)
                        *work++ &= ~pattern;
                    if (rightmask)
                        *work &= ~(pattern & rightmask);
                }
                break;
            case WM_XOR:
                *work ^= pattern & leftmask;
                work++;
                for (n = centre; n >= 0; n--)
                    *work++ ^= pattern;
                if (rightmask)
                    *work ^= pattern & rightmask;
                break;
            case WM_ERASE:
                if (bit) {
                    *work |= (~pattern) & leftmask;
                    work++;
                    for (n = centre; n >= 0; n--)
                        *work++ |= ~pattern;
                    if (rightmask)
                        *work |= (~pattern) & rightmask;
                } else {
                    *work &= ~((~pattern) & leftmask);
                    work++;
                    for (n = centre; n >= 0; n--)
                        *work++ &= pattern;
                    if (rightmask)
                        *work &= ~((~pattern) & rightmask);
                }
                break;
            }
            if (attr->multifill)
                patind += 16;
        }
    }
}

void amiga_draw_line(const Line *line, WORD wrt_mode, UWORD color)
{
    WORD x1 = line->x1, y1 = line->y1, x2 = line->x2, y2 = line->y2;
    WORD dx = x2 - x1;
    WORD dy = y2 - y1;
    WORD sx = (dx >= 0) ? 1 : -1;
    WORD sy = (dy >= 0) ? 1 : -1;
    WORD x, y, e2;
    WORD err;
    UWORD linemask = LN_MASK;

    if (dx < 0)
        dx = -dx;
    if (dy < 0)
        dy = -dy;
    err = dx - dy;
    x = x1;
    y = y1;

    for (;;) {
        rolw1(linemask);

        switch (wrt_mode) {
        case WM_REPLACE:
        default:
            amiga_pixelput(x, y, (linemask & 1) ? color : 0);
            break;
        case WM_TRANS:
            if (linemask & 1)
                amiga_pixelput(x, y, color);
            break;
        case WM_XOR:
            if (linemask & 1)
                amiga_pixelput(x, y, ~amiga_pixelread(x, y));
            break;
        case WM_ERASE:
            if (!(linemask & 1))
                amiga_pixelput(x, y, color);
            break;
        }

        if (x == x2 && y == y2)
            break;
        e2 = err << 1;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }

    LN_MASK = linemask;
}

#if CONF_WITH_VDI_VERTLINE
static void amiga_draw_vertical_line(const Line *line, WORD wrt_mode, UWORD color)
{
    amiga_draw_line(line, wrt_mode, color);
}
#endif

#if CONF_WITH_VDI_TEXT_SPEEDUP
static void amiga_blit_string(WORD count, WORD *str)
{
    WORD height = DELY;
    WORD src_width = FWIDTH;
    WORD dst_width = v_lin_wr;
    UBYTE *dst = (UBYTE *)amiga_plane_word(0, DESTX, DESTY) + ((DESTX & 0x0008) ? 1 : 0);

    while (count-- > 0) {
        UBYTE *src = (UBYTE *)FBASE + *str++;
        UBYTE *save_dst = dst;
        WORD n, plane;
        UBYTE *p, *qrow;
        UWORD fc = TEXTFG;

        for (n = height, p = src, qrow = save_dst; n > 0; n--, p += src_width, qrow += dst_width) {
            UBYTE glyph = *p;
            UWORD f = fc;

            for (plane = 0; plane < v_planes; plane++, f >>= 1) {
                UBYTE *q = amiga_plane_base(plane) + (ULONG)(qrow - v_bas_ad);

                switch (WRT_MODE) {
                case WM_REPLACE:
                default:
                    *q = (f & 1) ? glyph : 0;
                    break;
                case WM_TRANS:
                    if (f & 1)
                        *q |= glyph;
                    else
                        *q &= ~glyph;
                    break;
                case WM_XOR:
                    *q ^= glyph;
                    break;
                case WM_ERASE:
                    if (f & 1)
                        *q |= ~glyph;
                    else
                        *q &= glyph;
                    break;
                }
            }
        }
        dst = save_dst + 1;
    }
}
#endif

/*
 * Glyph blit without vdi_tblit.S (that advances planes by +2 for Atari
 * interleaved screens).  Handles the common monochrome font -> planar case.
 */
static void amiga_blit_glyph(LOCALVARS *vars)
{
    WORD height = vars->DELY;
    WORD width = vars->DELX;
    WORD srcx = SOURCEX & 0x000f;
    UBYTE *src = vars->sform
               + (SOURCEY * (LONG)vars->s_next)
               + ((SOURCEX >> 3) & ~1);
    WORD dx = vars->DESTX;
    WORD dy = vars->DESTY;
    WORD row, col;
    UWORD fg = TEXTFG;

    for (row = 0; row < height; row++) {
        for (col = 0; col < width; col++) {
            UWORD bit = (src[(col + srcx) >> 3] >> (7 - ((col + srcx) & 7))) & 1;
            WORD x = dx + col;
            WORD y = dy + row;

            if (x < 0 || y < 0 || x >= V_REZ_HZ || y >= V_REZ_VT)
                continue;

            switch (WRT_MODE) {
            case WM_REPLACE:
            default:
                amiga_pixelput(x, y, bit ? fg : 0);
                break;
            case WM_TRANS:
                if (bit)
                    amiga_pixelput(x, y, fg);
                break;
            case WM_XOR:
                if (bit)
                    amiga_pixelput(x, y, ~amiga_pixelread(x, y));
                break;
            case WM_ERASE:
                if (!bit)
                    amiga_pixelput(x, y, fg);
                break;
            }
        }
        src += vars->s_next;
    }
}

static void amiga_clear_screen(void)
{
    memset(v_bas_ad, 0x00, amiga_plane_size * v_planes);
}

static void amiga_fill_rect(const VwkAttrib *attr, const Rect *rect)
{
    amiga_swblit_rect(attr, rect);
}

const VDI_RASTER_DRIVER raster_driver_amiga_bitplanes = {
    .name = "Amiga bitplanes",
    .get_pixel = amiga_pixelread,
    .put_pixel = amiga_pixelput,
    .fill_rect = amiga_fill_rect,
    .draw_line = amiga_draw_line,
#if CONF_WITH_VDI_VERTLINE
    .draw_vertical_line = amiga_draw_vertical_line,
#endif
    .setup_forms = amiga_setup_forms,
    .copy_raster_opaque = vdi_copy_raster_opaque_planar,
    .copy_raster_transparent = vdi_copy_raster_transparent_planar,
    .blit = blit_frame_copy,
    .transform_form = vdi_transform_form_planar,
    .find_span = amiga_end_pts,
    .blit_glyph = amiga_blit_glyph,
#if CONF_WITH_VDI_TEXT_SPEEDUP
    .blit_string = amiga_blit_string,
#endif
    .clear_screen = amiga_clear_screen,
};

#endif /* MACHINE_AMIGA */
