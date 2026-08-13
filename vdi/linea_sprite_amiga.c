/*
 * linea_sprite_amiga.c - mouse sprite for Amiga planar framebuffers
 *
 * Copyright (C) 2026 The EmuTOS development team
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#include "emutos.h"

#if defined(MACHINE_AMIGA) && CONF_WITH_SOFTWARE_MOUSE_RENDERING

#include "linea.h"
#include "lineavars.h"
#include "tosvars.h"
#include "raster_driver_amiga_bitplanes.h"

#define MOUSE_HEIGHT    16

static void paint_clipped_sprite(WORD op, MCDB *sprite, MCS *mcs, UWORD *mask_start, UWORD shft);

void linea_sprite_show_amiga(MCDB *sprite, MCS *mcs, WORD x, WORD y)
{
    int row_count, plane, op;
    UWORD *mask_start;
    UWORD shft, cdb_fg, cdb_bg;
    UWORD cdb_mask;
    ULONG *save;
    WORD dst_inc;

    x -= sprite->xhot;
    y -= sprite->yhot;

    mcs->stat = 0x00;

    if (x < 0) {
        x += 16;
        op = 1;
    } else if (x >= (linea_max_x - 15)) {
        op = 2;
    } else {
        op = 0;
        mcs->stat |= MCS_LONGS;
    }

    mask_start = sprite->maskdata;
    if (y < 0) {
        row_count = y + 16;
        mask_start -= y << 1;
        y = 0;
    } else if (y > (linea_max_y - 15)) {
        row_count = linea_max_y - y + 1;
    } else {
        row_count = 16;
    }

    shft = 16 - (x & 0x0f);
    mcs->len = row_count;
    mcs->addr = amiga_plane_word(0, x, y);
    mcs->stat |= MCS_VALID;

    if (op) {
        paint_clipped_sprite(op, sprite, mcs, mask_start, shft);
        return;
    }

    dst_inc = v_lin_wr >> 1;
    save = mcs->area;
    cdb_bg = sprite->bg_col;
    cdb_fg = sprite->fg_col;

    for (plane = 0, cdb_mask = 0x0001; plane < v_planes; plane++, cdb_mask <<= 1) {
        int row;
        UWORD *src = mask_start;
        UWORD *dst = amiga_plane_word(plane, x, y);

        for (row = row_count - 1; row >= 0; row--) {
            ULONG bits, fg, bg;

            bits = ((ULONG)*dst) << 16;
            bits |= *(dst + 1);
            *save++ = bits;

            bg = (ULONG)*src++ << shft;
            fg = (ULONG)*src++ << shft;

            if (cdb_bg & cdb_mask)
                bits |= bg;
            else
                bits &= ~bg;

            if (cdb_fg & cdb_mask)
                bits |= fg;
            else
                bits &= ~fg;

            *dst = (UWORD)(bits >> 16);
            *(dst + 1) = (UWORD)bits;
            dst += dst_inc;
        }
    }
}

static void paint_clipped_sprite(WORD op, MCDB *sprite, MCS *mcs, UWORD *mask_start, UWORD shft)
{
    WORD dst_inc, plane;
    UWORD cdb_fg, cdb_bg, cdb_mask;
    UWORD *save;
    WORD x_word = ((ULONG)((UBYTE *)mcs->addr - v_bas_ad)) % v_lin_wr;
    WORD y = (WORD)(((ULONG)((UBYTE *)mcs->addr - v_bas_ad)) / v_lin_wr);
    WORD x = x_word << 3; /* approximate; plane0 word already accounts for x */

    /* Prefer stored address as plane-0 origin; recover x from word offset */
    x = (WORD)((((UBYTE *)mcs->addr - amiga_plane_base(0)) % v_lin_wr) << 3);

    dst_inc = v_lin_wr >> 1;
    save = (UWORD *)mcs->area;
    cdb_bg = sprite->bg_col;
    cdb_fg = sprite->fg_col;

    UNUSED(y);

    for (plane = 0, cdb_mask = 0x0001; plane < v_planes; plane++, cdb_mask <<= 1) {
        WORD row;
        UWORD *src = mask_start;
        UWORD *dst = (UWORD *)(amiga_plane_base(plane)
                               + ((UBYTE *)mcs->addr - amiga_plane_base(0)));

        for (row = mcs->len - 1; row >= 0; row--) {
            ULONG bits, fg, bg;

            *save++ = *dst;
            if (op == 1)
                bits = *dst;
            else
                bits = ((ULONG)*dst) << 16;

            bg = (ULONG)*src++ << shft;
            fg = (ULONG)*src++ << shft;

            if (cdb_bg & cdb_mask)
                bits |= bg;
            else
                bits &= ~bg;

            if (cdb_fg & cdb_mask)
                bits |= fg;
            else
                bits &= ~fg;

            if (op == 1)
                *dst = (UWORD)bits;
            else
                *dst = (UWORD)(bits >> 16);

            dst += dst_inc;
        }
    }

    UNUSED(x);
}

void linea_sprite_hide_amiga(MCS *mcs)
{
    WORD plane, row;
    UWORD *src;
    const WORD dst_inc = v_lin_wr >> 1;
    ULONG plane0_off;

    if (!(mcs->stat & MCS_VALID))
        return;
    mcs->stat &= ~MCS_VALID;

    plane0_off = (UBYTE *)mcs->addr - amiga_plane_base(0);
    src = (UWORD *)mcs->area;

    if (mcs->stat & MCS_LONGS) {
        for (plane = 0; plane < v_planes; plane++) {
            UWORD *dst = (UWORD *)(amiga_plane_base(plane) + plane0_off);

            for (row = mcs->len - 1; row >= 0; row--) {
                *dst = *src++;
                *(dst + 1) = *src++;
                dst += dst_inc;
            }
        }
        return;
    }

    for (plane = 0; plane < v_planes; plane++) {
        UWORD *dst = (UWORD *)(amiga_plane_base(plane) + plane0_off);

        for (row = mcs->len - 1; row >= 0; row--) {
            *dst = *src++;
            dst += dst_inc;
        }
    }
}

#endif /* MACHINE_AMIGA && CONF_WITH_SOFTWARE_MOUSE_RENDERING */
