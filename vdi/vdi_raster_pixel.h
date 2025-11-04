/*
 * vdi_fill.c - filled polygons
 *
 * Copyright 1982 by Digital Research Inc.  All rights reserved.
 * Copyright 1999 by Caldera, Inc. and Authors:
 * Copyright 2002-2025 The EmuTOS development team
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#ifndef _VDI_RASTER_PIXEL_H
#define _VDI_RASTER_PIXEL_H

#include "emutos.h"
#include "vdi_defs.h"

UWORD get_color (UWORD mask, UWORD * addr);
UWORD pixelread(const WORD x, const WORD y);
void pixelput(const WORD x, const WORD y);
#if CONF_WITH_VDI_16BIT
UWORD search_to_right16(const VwkClip *clip, WORD x, const UWORD search_col, UWORD *addr);
UWORD search_to_left16(const VwkClip *clip, WORD x, const UWORD search_col, UWORD *addr);
WORD end_pts16(const VwkClip *clip, WORD x, WORD y, UWORD search_color, BOOL seed_type, WORD *xleftout, WORD *xrightout);
#endif
UWORD search_to_right (const VwkClip * clip, WORD x, UWORD mask, const UWORD search_col, UWORD * addr);
UWORD search_to_left (const VwkClip * clip, WORD x, UWORD mask, const UWORD search_col, UWORD * addr);
WORD end_pts(const VwkClip *clip, WORD x, WORD y, UWORD search_color, BOOL seed_type,WORD *xleftout, WORD *xrightout);

#endif /* _VDI_RASTER_PIXEL_H */