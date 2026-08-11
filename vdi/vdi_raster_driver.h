/*
 * vdi_raster_driver.h - the framebuffer-format-specific half of the VDI
 *
 * In GDOS terms this is the "screen driver": everything that knows how
 * pixels are laid out in the framebuffer.  The Atari ST/STe/TT use
 * interleaved bitplanes, the Falcon Truecolor modes use packed 16-bit
 * pixels, and the Foenix machines use chunky 8bpp; each of those is a
 * separate implementation of the structure below.
 *
 * Everything above this interface (VDI opcode dispatch, clipping,
 * polygons, wide lines, fonts, attributes, colour mapping) is
 * device-independent and must not make assumptions about the pixel
 * layout.
 *
 * Note that this interface is deliberately NOT per-pixel: entries take a
 * whole span, glyph or blit so that the inner loops stay inside the
 * driver.  Pixel address computation (get_start_addr() in vdi_inline.h)
 * likewise stays inline and private to each driver.
 *
 * Copyright (C) 2025 The EmuTOS development team
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#ifndef _VDI_RASTER_DRIVER_H
#define _VDI_RASTER_DRIVER_H

#include "emutos.h"
#include "vdi_defs.h"
#include "vdi_blit.h"
#include "vdi_textblit.h"

typedef struct {
    const char *name;               /* for KDEBUG only */

    /* --- pixels --- */
    UWORD (*get_pixel)(WORD x, WORD y);
    void (*put_pixel)(WORD x, WORD y, UWORD color);

    /* --- spans, rectangles, patterned fills --- */
    void (*fill_rect)(const VwkAttrib *attr, const Rect *rect);

    /* --- lines --- */
    void (*draw_line)(const Line *line, WORD wrt_mode, UWORD color);
#if CONF_WITH_VDI_VERTLINE
    void (*draw_vertical_line)(const Line *line, WORD wrt_mode, UWORD color);
#endif

    /*
     * --- raster copy ---
     *
     * setup_forms() fills in the plane-layout members of 'info' from the
     * source & destination MFDBs (a NULL fd_addr means the screen).  It
     * returns TRUE if the resulting plane count cannot be handled.
     *
     * Note that copy_raster_*() dispatch on the plane count of the forms
     * involved, not on the current screen mode: a Truecolor screen can
     * still be the destination of a blit from a low-plane-count memory
     * form, and vice versa.
     */
    BOOL (*setup_forms)(struct blit_frame *info, const MFDB *src, const MFDB *dst);
    void (*copy_raster_opaque)(struct blit_frame *info);
    void (*copy_raster_transparent)(struct blit_frame *info);
    void (*blit)(struct blit_frame *info);          /* line-A $7 raw blit */
    void (*transform_form)(MFDB *src, MFDB *dst);   /* vr_trnfm */

    /* --- flood fill probe: find the extent of a run of one colour --- */
    WORD (*find_span)(const VwkClip *clip, WORD x, WORD y, UWORD search_color,
                      BOOL seed_type, WORD *xleftout, WORD *xrightout);

    /* --- text --- */
    void (*blit_glyph)(LOCALVARS *vars);            /* one character */
#if CONF_WITH_VDI_TEXT_SPEEDUP
    void (*blit_string)(WORD count, WORD *str);     /* byte-aligned 8-pixel mono font */
#endif

    /* --- misc --- */
    void (*clear_screen)(void);
} VDI_RASTER_DRIVER;

/*
 * The active driver.
 *
 * This is a pointer rather than a copied structure so that it can be
 * statically initialised: line-A entry points can be reached from the
 * BIOS console before any workstation has been opened, so there must
 * never be a window during which the entries are NULL.
 */
extern const VDI_RASTER_DRIVER *vdi_raster;

/* choose the driver matching the current framebuffer format */
void vdi_raster_select(void);

/* the available implementations */
extern const VDI_RASTER_DRIVER raster_driver_atari_bitplanes;
#if CONF_WITH_VDI_16BIT
extern const VDI_RASTER_DRIVER raster_driver_atari_truecolor;
#endif
#if CONF_WITH_CHUNKY8
extern const VDI_RASTER_DRIVER raster_driver_chunky8;
#endif

#endif /* _VDI_RASTER_DRIVER_H */
