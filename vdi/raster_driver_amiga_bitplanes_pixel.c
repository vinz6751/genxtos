/*
 * raster_driver_amiga_bitplanes_pixel.c - pixel ops for Amiga planar screens
 *
 * Copyright (C) 2026 The EmuTOS development team
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#include "emutos.h"

#ifdef MACHINE_AMIGA

#include "lineavars.h"
#include "tosvars.h"
#include "raster_driver_amiga_bitplanes.h"

UWORD amiga_pixelread(WORD x, WORD y)
{
    UWORD color = 0;
    UWORD mask = 0x8000 >> (x & 0x0f);
    WORD plane;

    for (plane = v_planes - 1; plane >= 0; plane--) {
        color <<= 1;
        if (*amiga_plane_word(plane, x, y) & mask)
            color |= 1;
    }
    return color;
}

void amiga_pixelput(WORD x, WORD y, UWORD color)
{
    UWORD mask = 0x8000 >> (x & 0x0f);
    WORD plane;

    if (x < 0 || y < 0 || x >= V_REZ_HZ || y >= V_REZ_VT)
        return;

    for (plane = 0; plane < v_planes; plane++) {
        UWORD *addr = amiga_plane_word(plane, x, y);

        if (color & 0x0001)
            *addr |= mask;
        else
            *addr &= ~mask;
        color >>= 1;
    }
}

WORD amiga_end_pts(const VwkClip *clip, WORD x, WORD y, UWORD search_color, BOOL seed_type,
                   WORD *xleftout, WORD *xrightout)
{
    UWORD color;
    WORD xl, xr;

    if (y < clip->ymn_clip || y > clip->ymx_clip)
        return 0;

    color = amiga_pixelread(x, y);
    xl = x;
    while (xl > clip->xmn_clip && amiga_pixelread(xl - 1, y) == color)
        xl--;
    xr = x;
    while (xr < clip->xmx_clip && amiga_pixelread(xr + 1, y) == color)
        xr++;

    *xleftout = xl;
    *xrightout = xr;

    if (color != search_color)
        return seed_type ^ 1;
    return seed_type ^ 0;
}

#endif /* MACHINE_AMIGA */
