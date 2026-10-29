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

#if CONF_WITH_ATARI_VIDEO

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
 * advance destination pointer to the next character cell
 */
static UBYTE *blit_string_next_dst(UBYTE *save_dst)
{
    UBYTE *dst = save_dst + 1;

    if (!IS_ODD_POINTER(dst))   /* must go to next screen word */
        dst += (v_planes - 1) * sizeof(WORD);
    return dst;
}

/* one bus word; the pointer is even */
static void store_plane_word(UBYTE *p, UWORD bits)
{
    __asm__ volatile ("move.w %0,(%1)" :: "d"(bits), "a"(p) : "memory");
}

/*
 * WM_REPLACE, one 8px cell.  fill[plane] is 0xFF or 0x00, computed once
 * from TEXTFG so the scanline loop does not test the colour.
 *
 * noinline: gcc 4.6 inlines this into bitplanes_blit_string and then
 * keeps the plane test in the scanline loop, which is slower than the
 * plain byte copy on a 1-plane screen.
 */
static void __attribute__((noinline)) replace_one_cell(UBYTE *dst, const UBYTE *src,
                             WORD height, WORD src_width, WORD dst_width,
                             WORD planes, const UBYTE *fill)
{
    WORD n;
    const UBYTE *p;
    UBYTE *qrow;

    if (planes == 1) {
        if (fill[0]) {
            for (n = height, p = src, qrow = dst; n > 0; n--, p += src_width, qrow += dst_width)
                *qrow = *p;
        } else {
            for (n = height, qrow = dst; n > 0; n--, qrow += dst_width)
                *qrow = 0;
        }
        return;
    }

    for (n = height, p = src, qrow = dst; n > 0; n--, p += src_width, qrow += dst_width) {
        UBYTE glyph = *p;

        if (planes == 4) {
            qrow[0] = glyph & fill[0];
            qrow[2] = glyph & fill[1];
            qrow[4] = glyph & fill[2];
            qrow[6] = glyph & fill[3];
        } else {
            UBYTE *q = qrow;
            WORD i;

            for (i = 0; i < planes; i++, q += sizeof(WORD))
                *q = glyph & fill[i];
        }
    }
}

/*
 * WM_REPLACE, two 8px cells that share one plane word.
 * High byte is the even cell (pixels 0-7), low byte the odd cell.
 *
 * The row loops are dbra so a 1-plane string is a word store per scanline
 * and a 4-plane string is four adjacent word stores.  gcc 4.6 -Os will not
 * keep that shape if these are written as ordinary C and then inlined.
 */
static void __attribute__((noinline))
replace_pair_mono(UBYTE *dst, const UBYTE *s0, const UBYTE *s1,
                  WORD height, WORD src_width, WORD dst_width, int ink)
{
    WORD h = height - 1;

    if (h < 0)
        return;

    if (ink) {
        __asm__ volatile (
            "1:\n\t"
            "moveq #0,%%d0\n\t"
            "move.b (%1),%%d0\n\t"
            "lsl.w #8,%%d0\n\t"
            "move.b (%2),%%d0\n\t"
            "move.w %%d0,(%0)\n\t"
            "adda.w %4,%1\n\t"
            "adda.w %4,%2\n\t"
            "adda.w %5,%0\n\t"
            "dbra %3,1b"
            : "+a"(dst), "+a"(s0), "+a"(s1), "+d"(h)
            : "d"(src_width), "d"(dst_width)
            : "d0", "memory", "cc"
        );
    } else {
        __asm__ volatile (
            "1:\n\t"
            "clr.w (%0)\n\t"
            "adda.w %2,%0\n\t"
            "dbra %1,1b"
            : "+a"(dst), "+d"(h)
            : "d"(dst_width)
            : "memory", "cc"
        );
    }
}

static void __attribute__((noinline))
replace_pair_4(UBYTE *dst, const UBYTE *s0, const UBYTE *s1,
               WORD height, WORD src_width, WORD dst_width,
               UWORD m0, UWORD m1, UWORD m2, UWORD m3)
{
    WORD h = height - 1;

    if (h < 0)
        return;

    __asm__ volatile (
        "1:\n\t"
        "moveq #0,%%d0\n\t"
        "move.b (%0),%%d0\n\t"
        "lsl.w #8,%%d0\n\t"
        "move.b (%1),%%d0\n\t"
        "move.w %%d0,%%d1\n\t"
        "and.w %6,%%d1\n\t"
        "move.w %%d1,(%2)\n\t"
        "move.w %%d0,%%d1\n\t"
        "and.w %7,%%d1\n\t"
        "move.w %%d1,2(%2)\n\t"
        "move.w %%d0,%%d1\n\t"
        "and.w %8,%%d1\n\t"
        "move.w %%d1,4(%2)\n\t"
        "and.w %9,%%d0\n\t"
        "move.w %%d0,6(%2)\n\t"
        "adda.w %4,%0\n\t"
        "adda.w %4,%1\n\t"
        "adda.w %5,%2\n\t"
        "dbra %3,1b"
        : "+a"(s0), "+a"(s1), "+a"(dst), "+d"(h)
        : "g"(src_width), "g"(dst_width), "m"(m0), "m"(m1), "m"(m2), "m"(m3)
        : "d0", "d1", "memory", "cc"
    );
}

static void __attribute__((noinline))
replace_word_pair(UBYTE *dst, const UBYTE *s0, const UBYTE *s1,
                  WORD height, WORD src_width, WORD dst_width,
                  WORD planes, const UWORD *wmask)
{
    WORD n;

    for (n = height; n > 0; n--, s0 += src_width, s1 += src_width, dst += dst_width) {
        UWORD bits = ((UWORD)s0[0] << 8) | s1[0];
        WORD i;

        for (i = 0; i < planes; i++)
            store_plane_word(dst + i * (WORD)sizeof(WORD), bits & wmask[i]);
    }
}

/*
 * WM_REPLACE for the whole string.
 *
 * An 8px cell is one byte of each plane word.  Two consecutive cells that
 * start on an even byte are one word store per plane per row.  A string
 * that starts on the odd byte writes that first cell alone, then pairs.
 */
static void __attribute__((noinline)) blit_string_replace(WORD count, WORD *str, UBYTE *dst)
{
    WORD planes = v_planes;
    WORD height = DELY;
    WORD src_width = FWIDTH;
    WORD dst_width = v_lin_wr;
    UBYTE fill[8];
    UWORD wmask[8];
    WORD i;
    WORD ink;
    UWORD bit;

    /* Atari interleaved modes used here are 1, 2, 4 or 8 planes. */
    if (planes > 8)
        planes = 8;

    for (i = 0, bit = 1; i < planes; i++, bit <<= 1) {
        if (TEXTFG & bit) {
            fill[i] = 0xff;
            wmask[i] = 0xffff;
        } else {
            fill[i] = 0;
            wmask[i] = 0;
        }
    }

    if (IS_ODD_POINTER(dst) && count > 0) {
        replace_one_cell(dst, (const UBYTE *)FBASE + *str++,
                         height, src_width, dst_width, planes, fill);
        dst = blit_string_next_dst(dst);
        count--;
    }

    if (planes == 1) {
        ink = fill[0] != 0;

        while (count >= 2) {
            const UBYTE *s0 = (const UBYTE *)FBASE + *str++;
            const UBYTE *s1 = (const UBYTE *)FBASE + *str++;

            replace_pair_mono(dst, s0, s1, height, src_width, dst_width, ink);
            dst += (WORD)sizeof(WORD);
            count -= 2;
        }
    } else if (planes == 4) {
        while (count >= 2) {
            const UBYTE *s0 = (const UBYTE *)FBASE + *str++;
            const UBYTE *s1 = (const UBYTE *)FBASE + *str++;

            replace_pair_4(dst, s0, s1, height, src_width, dst_width,
                           wmask[0], wmask[1], wmask[2], wmask[3]);
            dst += 4 * (WORD)sizeof(WORD);
            count -= 2;
        }
    } else {
        while (count >= 2) {
            const UBYTE *s0 = (const UBYTE *)FBASE + *str++;
            const UBYTE *s1 = (const UBYTE *)FBASE + *str++;

            replace_word_pair(dst, s0, s1, height, src_width, dst_width, planes, wmask);
            dst += planes * (WORD)sizeof(WORD);
            count -= 2;
        }
    }

    if (count > 0) {
        replace_one_cell(dst, (const UBYTE *)FBASE + *str,
                         height, src_width, dst_width, planes, fill);
    }
}

/*
 * output a character string directly to the screen
 *
 * fast path for a monospaced 8-pixel-wide mono font output byte-aligned
 * with no special effects; see ok_for_direct_blit()
 */
static void bitplanes_blit_string(WORD count, WORD *str)
{
    WORD height, src_width, dst_width, n, planes;
    UBYTE *src, *dst, *save_dst, *p, *q, *qrow;
    UBYTE glyph;
    WORD fc;

    height = DELY;
    src_width = FWIDTH;
    dst_width = v_lin_wr;

    dst = (UBYTE *)get_start_addr(DESTX, DESTY);

    /* get_start_addr() is word-aligned; the odd byte holds pixels 8-15 */
    if (DESTX & 0x0008)
        dst++;

    switch (WRT_MODE) {
    default:    /* WM_REPLACE */
        blit_string_replace(count, str, dst);
        break;
    case WM_TRANS:
        for ( ; count > 0; count--)
        {
            src = (UBYTE *)FBASE + *str++;
            save_dst = dst;
            for (n = height, p = src, qrow = save_dst; n > 0; n--, p += src_width, qrow += dst_width)
            {
                glyph = *p;
                q = qrow;
                fc = TEXTFG;
                for (planes = v_planes; planes > 0; planes--, q += sizeof(WORD), fc >>= 1)
                {
                    if (fc & 1)
                        *q |= glyph;
                    else
                        *q &= ~glyph;
                }
            }
            dst = blit_string_next_dst(save_dst);
        }
        break;
    case WM_XOR:
        for ( ; count > 0; count--)
        {
            src = (UBYTE *)FBASE + *str++;
            save_dst = dst;
            for (n = height, p = src, qrow = save_dst; n > 0; n--, p += src_width, qrow += dst_width)
            {
                glyph = *p;
                q = qrow;
                for (planes = v_planes; planes > 0; planes--, q += sizeof(WORD))
                    *q ^= glyph;
            }
            dst = blit_string_next_dst(save_dst);
        }
        break;
    case WM_ERASE:
        for ( ; count > 0; count--)
        {
            src = (UBYTE *)FBASE + *str++;
            save_dst = dst;
            for (n = height, p = src, qrow = save_dst; n > 0; n--, p += src_width, qrow += dst_width)
            {
                glyph = *p;
                q = qrow;
                fc = TEXTFG;
                for (planes = v_planes; planes > 0; planes--, q += sizeof(WORD), fc >>= 1)
                {
                    if (fc & 1)
                        *q |= ~glyph;
                    else
                        *q &= glyph;
                }
            }
            dst = blit_string_next_dst(save_dst);
        }
        break;
    }
}
#endif

static void bitplanes_blit_glyph(LOCALVARS *vars)
{
    LONG offset;

    vars->forecol = TEXTFG;
    vars->ambient = 0;          /* logically TEXTBG, but that isn't set up by the VDI */
    vars->nbrplane = v_planes;
    vars->nextwrd = vars->nbrplane * (WORD)sizeof(WORD);
    vars->height = vars->DELY;
    vars->width = vars->DELX;

    /*
     * calculate the starting address for the character to be copied
     */
    vars->tsdad = SOURCEX & 0x000f; /* source dot address */
    offset = (SOURCEY+vars->DELY-1) * (LONG)vars->s_next
             + ((SOURCEX >> 3) & ~1);
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
    vars->dform += (vars->DESTX&0xfff0)>>v_planes_shift; /* add x coordinate part of addr */
    vars->dform += (UWORD)(vars->DESTY+vars->DELY-1) * (ULONG)v_lin_wr; /* add y coordinate part of addr */
    vars->d_next = -v_lin_wr;

    normal_blit(vars+1, vars->sform, vars->dform);
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

#endif /* CONF_WITH_ATARI_VIDEO */
