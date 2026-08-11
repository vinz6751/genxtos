/*
 * vdi_raster_bitplane.c - VDI screen raster driver for Atari bitplanes
 *
 * This is the reconstruction of the Atari ST "screen driver": the half of
 * the VDI that knows the framebuffer is made of interleaved bitplanes, as
 * used by the ST, STe, TT and the Falcon's palette-based modes.
 *
 * In a full GDOS the equivalent code would have lived in a loadable
 * SCREEN.SYS; Atari kept it in ROM (devices 01-04 in ASSIGN.SYS are
 * flagged "permanent" for exactly that reason), which is why it ended up
 * intertwined with the device-independent VDI.  See vdi_raster_driver.h.
 *
 * This file holds the driver structure itself, the text output entries,
 * and the odds and ends; the bulk of the drawing code is in
 * vdi_raster_bitplane_line.c and vdi_raster_bitplane_pixel.c.
 *
 * Copyright 1999 by Caldera, Inc. and Authors:
 * Copyright 2002-2025 The EmuTOS development team
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

/* #define ENABLE_KDEBUG */

#include "emutos.h"
#include "asm.h"
#include "intmath.h"
#include "aesext.h"
#include "has.h"        /* for blitter-related items */
#include "string.h"     /* for memset() */
#include "blitter.h"
#include "tosvars.h"
#include "lineavars.h"
#include "vdi_defs.h"
#include "vdistub.h"
#include "vdi_blit.h"
#include "vdi_inline.h"
#include "vdi_textblit.h"
#include "vdi_raster_bitplane.h"
#include "vdi_raster_driver.h"

#if CONF_WITH_VDI_TEXT_SPEEDUP
#include "vdi_raster_bitplane_blit_string.inc"

static void bp_blit_string(WORD count, WORD *str)
{
    BP_BLIT_STRING(count, str);
}
#endif

#include "vdi_raster_bitplane_blit_glyph.inc"

static void bp_blit_glyph(LOCALVARS *vars)
{
    BP_BLIT_GLYPH(vars);
}


/*
 * ===========================================================================
 * VDI_RASTER_DRIVER entries
 * ===========================================================================
 */

/*
 * the hardware blitter is an accelerator for the bitplane layout, so the
 * choice between it and the software loops belongs to this driver rather
 * than to the device-independent VDI.
 */
static void bp_fill_rect(const VwkAttrib *attr, const Rect *rect)
{
#if CONF_WITH_BLITTER
    if (blitter_is_enabled)
    {
        hwblit_rect_common(attr, rect);
        return;
    }
#endif

    swblit_rect_common(attr, rect);
}


#if CONF_WITH_VDI_VERTLINE
static void bp_draw_vertical_line(const Line *line, WORD wrt_mode, UWORD color)
{
#if CONF_WITH_BLITTER
    if (blitter_is_enabled)
    {
        hwblit_vertical_line(line, wrt_mode, color);
        return;
    }
#endif

    vertical_line(line, wrt_mode, color);
}
#endif


static void bp_clear_screen(void)
{
    memset(v_bas_ad, 0x00, (ULONG)v_lin_wr * V_REZ_VT);
}


const VDI_RASTER_DRIVER vdi_raster_bitplane = {
    .name = "Atari bitplanes",
    .resolution_changed = NULL,
    .get_pixel = pixelread,
    .put_pixel = pixelput,
    .fill_rect = bp_fill_rect,
    .draw_line = draw_line,
#if CONF_WITH_VDI_VERTLINE
    .draw_vertical_line = bp_draw_vertical_line,
#endif
    .setup_forms = vdi_setup_forms_planar,
    .copy_raster_opaque = vdi_copy_raster_opaque_planar,
    .copy_raster_transparent = vdi_copy_raster_transparent_planar,
    .blit = blit_frame_copy,
    .transform_form = vdi_transform_form_planar,
    .find_span = end_pts,
    .blit_glyph = bp_blit_glyph,
#if CONF_WITH_VDI_TEXT_SPEEDUP
    .blit_string = bp_blit_string,
#endif
    .clear_screen = bp_clear_screen,
};
