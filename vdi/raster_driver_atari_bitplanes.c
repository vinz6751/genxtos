/*
 * raster_driver_atari_bitplanes.c - VDI screen raster driver for Atari bitplanes
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
 * raster_driver_atari_bitplanes_line.c and raster_driver_atari_bitplanes_pixel.c.
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
#include "raster_driver_atari_bitplanes.h"
#include "vdi_raster_driver.h"

#if CONF_WITH_VDI_TEXT_SPEEDUP
/*
 * output a character string directly to the screen
 *
 * fast path for a monospaced 8-pixel-wide mono font output byte-aligned
 * with no special effects; see ok_for_direct_blit()
 */
static void bitplanes_blit_string(WORD count, WORD *str)
{
    WORD forecol, height, mode, n, planes;
    WORD src_width, dst_width;
    UBYTE *src, *dst, *save_dst;

    height = DELY;
    mode = WRT_MODE;
    src_width = FWIDTH;
    dst_width = v_lin_wr;

    dst = (UBYTE *)get_start_addr(DESTX, DESTY);

    if ((DESTX & 0x0008) && (v_planes != 1))
        dst++;

    for ( ; count > 0; count--)
    {
        src = (UBYTE *)FBASE + *str++;
        save_dst = dst;
        forecol = TEXTFG;
        for (planes = v_planes; planes > 0; planes--)
        {
            UBYTE *p, *q;

            switch(mode) {
            default:    /* WM_REPLACE */
                if (forecol & 1)
                {
                    for (n = height, p = src, q = dst; n > 0; n--)
                    {
                        *q = *p;
                        p += src_width;
                        q += dst_width;
                    }
                }
                else
                {
                    for (n = height, q = dst; n > 0; n--)
                    {
                        *q = 0;
                        q += dst_width;
                    }
                }
                break;
            case WM_TRANS:
                if (forecol & 1)
                {
                    for (n = height, p = src, q = dst; n > 0; n--)
                    {
                        *q |= *p;
                        p += src_width;
                        q += dst_width;
                    }
                }
                else
                {
                    for (n = height, p = src, q = dst; n > 0; n--)
                    {
                        *q &= ~*p;
                        p += src_width;
                        q += dst_width;
                    }
                }
                break;
            case WM_XOR:
                for (n = height, p = src, q = dst; n > 0; n--)
                {
                    *q ^= *p;
                    p += src_width;
                    q += dst_width;
                }
                break;
            case WM_ERASE:
                if (forecol & 1)
                {
                    for (n = height, p = src, q = dst; n > 0; n--)
                    {
                        *q |= ~*p;
                        p += src_width;
                        q += dst_width;
                    }
                }
                else
                {
                    for (n = height, p = src, q = dst; n > 0; n--)
                    {
                        *q &= *p;
                        p += src_width;
                        q += dst_width;
                    }
                }
                break;
            }
            dst += sizeof(WORD);    /* next plane */
            forecol >>= 1;
        }
        dst = save_dst + 1;
        if (!IS_ODD_POINTER(dst))   /* must go to next screen word */
        {
            dst += (v_planes-1)*sizeof(WORD);
        }
    }
}
#endif

static void bitplanes_blit_glyph(LOCALVARS *bitplanes_vars)
{
#include "raster_driver_atari_bitplanes_blit_glyph_body.c"
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
static void bitplanes_fill_rect(const VwkAttrib *attr, const Rect *rect)
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
static void bitplanes_draw_vertical_line(const Line *line, WORD wrt_mode, UWORD color)
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


static void bitplanes_clear_screen(void)
{
    memset(v_bas_ad, 0x00, (ULONG)v_lin_wr * V_REZ_VT);
}


const VDI_RASTER_DRIVER raster_driver_atari_bitplanes = {
    .name = "Atari bitplanes",
    .get_pixel = pixelread,
    .put_pixel = pixelput,
    .fill_rect = bitplanes_fill_rect,
    .draw_line = draw_line,
#if CONF_WITH_VDI_VERTLINE
    .draw_vertical_line = bitplanes_draw_vertical_line,
#endif
    .setup_forms = vdi_setup_forms_planar,
    .copy_raster_opaque = vdi_copy_raster_opaque_planar,
    .copy_raster_transparent = vdi_copy_raster_transparent_planar,
    .blit = blit_frame_copy,
    .transform_form = vdi_transform_form_planar,
    .find_span = end_pts,
    .blit_glyph = bitplanes_blit_glyph,
#if CONF_WITH_VDI_TEXT_SPEEDUP
    .blit_string = bitplanes_blit_string,
#endif
    .clear_screen = bitplanes_clear_screen,
};
