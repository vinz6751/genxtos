/*
 * raster_driver_amiga_bitplanes.h - Amiga planar VDI raster helpers
 *
 * Copyright (C) 2026 The EmuTOS development team
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#ifndef _RASTER_DRIVER_AMIGA_BITPLANES_H
#define _RASTER_DRIVER_AMIGA_BITPLANES_H

#include "emutos.h"
#include "vdi_defs.h"
#include "lineavars.h"

extern ULONG amiga_plane_size;
UBYTE *amiga_plane_base(WORD plane);

/* Word address of pixel x,y within one Amiga bitplane */
static __inline__ UWORD *amiga_plane_word(WORD plane, WORD x, WORD y)
{
    return (UWORD *)(amiga_plane_base(plane) + (UWORD)y * v_lin_wr + ((x & ~0x000f) >> 3));
}

UWORD amiga_pixelread(WORD x, WORD y);
void amiga_pixelput(WORD x, WORD y, UWORD color);
void amiga_swblit_rect(const VwkAttrib *attr, const Rect *rect);
void amiga_draw_line(const Line *line, WORD wrt_mode, UWORD color);
WORD amiga_end_pts(const VwkClip *clip, WORD x, WORD y, UWORD search_color, BOOL seed_type,
                   WORD *xleftout, WORD *xrightout);

#endif /* _RASTER_DRIVER_AMIGA_BITPLANES_H */
