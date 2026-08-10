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
/*
 * output a character string directly to the screen
 *
 * this is used for the special (but common) case of a string with no
 * special effects, no rotation, no justification, left-alignment,
 * byte-aligned output, using a monospaced font with a cell width of 8
 *
 * note: like Atari TOS, we assume that the font contains the full
 * character set, i.e. first_ade==0, last_ade==255
 */
static void bp_blit_string(WORD count, WORD *str)
{
    WORD forecol, height, mode, n, planes;
    WORD src_width, dst_width;
    UBYTE *src, *dst, *save_dst;

    height = DELY;
    mode = WRT_MODE;
    src_width = FWIDTH;
    dst_width = v_lin_wr;

    dst = (UBYTE *)get_start_addr(DESTX, DESTY);

    if (DESTX & 0x0008)
    {
#if CONF_WITH_CHUNKY8
        /*
         * wart for the Foenix packed-1bpp bring-up mode on VICKY B, which
         * is the one single-plane mode reaching this driver on a chunky
         * machine: there get_start_addr() already yields the correct byte,
         * and advancing would skip an 8-pixel cell between strings (e.g.
         * menu titles).  This does not apply to Atari mono.
         */
        if (v_planes != 1)
#endif
            dst++;
    }

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


/*
 * output a glyph to the screen
 *
 * this is called each time a character is output to the screen, unless
 * CONF_WITH_VDI_TEXT_SPEEDUP is configured, and the string to be output
 * has no special features, in which case bp_blit_string() is used to
 * output a whole string at once.
 */
static void bp_blit_glyph(LOCALVARS *vars)
{
    LONG offset;

    vars->forecol = TEXTFG;
    vars->ambient = 0;          /* logically TEXTBG, but that isn't set up by the VDI */
    vars->nbrplane = v_planes;
    vars->nextwrd = vars->nbrplane * sizeof(WORD);
    vars->height = vars->DELY;
    vars->width = vars->DELX;

    /*
     * calculate the starting address for the character to be copied
     */
    vars->tsdad = SOURCEX & 0x000f; /* source dot address */
    offset = (SOURCEY+vars->DELY-1) * (LONG)vars->s_next + ((SOURCEX >> 3) & ~1);
    vars->sform += offset;
    vars->s_next = -vars->s_next;   /* we draw from the bottom up */

    /*
     * calculate the screen address
     *
     * note that the casts below allow the compiler to generate a mulu
     * instruction rather than calling _mulsi3(): this by itself speeds
     * up plain text output by about 3% ...
     */
    vars->tddad = vars->DESTX & 0x000f;
    vars->dform = v_bas_ad;
    vars->dform += (vars->DESTX&0xfff0)>>v_planes_shift;    /* add x coordinate part of addr */
    vars->dform += (UWORD)(vars->DESTY+vars->DELY-1) * (ULONG)v_lin_wr; /* add y coordinate part of addr */
    vars->d_next = -v_lin_wr;

    normal_blit(vars+1, vars->sform, vars->dform);  /* call assembler helper function */
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
