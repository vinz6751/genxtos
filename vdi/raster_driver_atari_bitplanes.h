/*
 * raster_driver_atari_bitplanes.h - internals of the Atari bitplane raster driver
 *
 * These are the primitives that make up the interleaved-bitplane
 * implementation of VDI_RASTER_DRIVER.  They are split over several
 * source files purely for readability:
 *
 *  raster_driver_atari_bitplanes.c       driver structure, text output, misc
 *  raster_driver_atari_bitplanes_line.c  lines, rectangles, patterned fills
 *  raster_driver_atari_bitplanes_pixel.c pixels and flood-fill probing
 *
 * Nothing outside those files (and vdi_raster_driver.c) should call these
 * directly: the rest of the VDI goes through the driver structure.
 *
 * Copyright (C) 2025 The EmuTOS development team
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#ifndef _RASTER_DRIVER_ATARI_BITPLANES_H
#define _RASTER_DRIVER_ATARI_BITPLANES_H

#include "emutos.h"
#include "vdi_defs.h"

/* lines, rectangles & patterned fills */
#if CONF_WITH_BLITTER
void hwblit_rect_nonstd(const VwkAttrib *attr, const Rect *rect, UWORD *addr);
void hwblit_rect_common(const VwkAttrib *attr, const Rect *rect);
#if CONF_WITH_VDI_VERTLINE
void hwblit_vertical_line(const Line *line, WORD wrt_mode, UWORD color);
#endif
#endif

void OPTIMIZE_SMALL swblit_rect_common(const VwkAttrib *attr, const Rect *rect);
void draw_line(const Line *line, WORD wrt_mode, UWORD color);
#if CONF_WITH_VDI_VERTLINE
void vertical_line(const Line *line, WORD wrt_mode, UWORD color);
#endif

/* pixels & flood-fill probing */
UWORD get_color(UWORD mask, UWORD *addr);
UWORD pixelread(WORD x, WORD y);
void pixelput(WORD x, WORD y, UWORD color);
UWORD search_to_right(const VwkClip *clip, WORD x, UWORD mask, const UWORD search_col, UWORD *addr);
UWORD search_to_left(const VwkClip *clip, WORD x, UWORD mask, const UWORD search_col, UWORD *addr);
WORD end_pts(const VwkClip *clip, WORD x, WORD y, UWORD search_color, BOOL seed_type,
             WORD *xleftout, WORD *xrightout);

#endif /* _RASTER_DRIVER_ATARI_BITPLANES_H */
