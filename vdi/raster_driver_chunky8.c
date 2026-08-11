/*
 * vdi_raster_chunky8.c - VDI screen raster driver for chunky 8bpp
 *
 * This is the Foenix (A2560U/K/X, GenX) implementation of
 * VDI_RASTER_DRIVER: one byte per pixel holding a palette index, so
 * there are neither bitplanes to walk nor a palette lookup to do when
 * writing - the colour index *is* the pixel value.
 *
 * It is structurally the same as vdi_raster_truecolor.c (which is also
 * a packed format) with UBYTE in place of UWORD.
 *
 * Not yet chunky-aware: the raster copy entries.  A memory MFDB is
 * planar even on these machines (the AES builds them with fd_nplanes ==
 * v_planes), so a chunky-aware vro_cpyfm/vrt_cpyfm has to convert
 * between the two layouts.  Until that is written those entries behave
 * exactly as they did before this driver existed, i.e. they describe the
 * screen as planar and use the shared word blit engine.
 *
 * Copyright (C) 2025 The EmuTOS development team
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

/* #define ENABLE_KDEBUG */

#include "emutos.h"

#if CONF_WITH_CHUNKY8

#include "asm.h"
#include "intmath.h"
#include "aesext.h"
#include "string.h"     /* for memset() */
#include "tosvars.h"
#include "lineavars.h"
#include "vdi_defs.h"
#include "vdistub.h"
#include "vdi_blit.h"
#include "vdi_inline.h"
#include "vdi_textblit.h"
#include "vdi_raster_driver.h"

#define OUTLINE_THICKNESS   1   /* must agree with vdi_textblit.c */


/*
 * ---------------------------------------------------------------------------
 * pixels
 * ---------------------------------------------------------------------------
 */

static UWORD ch8_get_pixel(WORD x, WORD y)
{
    return *(UBYTE *)get_start_addr(x, y);
}


static void ch8_put_pixel(WORD x, WORD y, UWORD color)
{
    UBYTE *addr = (UBYTE *)get_start_addr(x, y);

    /* co-ordinates can wrap, but we cannot write outside the screen */
    if (addr < (UBYTE *)v_bas_ad || addr >= (UBYTE *)get_start_addr(V_REZ_HZ, V_REZ_VT))
        return;

    *addr = (UBYTE)color;
}


/*
 * find the extent of the run of pixels having the same colour as (x,y)
 */
static WORD ch8_find_span(const VwkClip *clip, WORD x, WORD y, UWORD search_color,
                          BOOL seed_type, WORD *xleftout, WORD *xrightout)
{
    UBYTE *addr, *p;
    UBYTE color;
    WORD i;

    /* see if we are in the y clipping range */
    if (y < clip->ymn_clip || y > clip->ymx_clip)
        return 0;

    addr = (UBYTE *)get_start_addr(x, y);
    color = *addr;

    /* scan to the right */
    for (i = x, p = addr; i <= clip->xmx_clip; i++)
        if (*p++ != color)
            break;
    *xrightout = i - 1;

    /* scan to the left */
    for (i = x, p = addr; i >= clip->xmn_clip; i--)
        if (*p-- != color)
            break;
    *xleftout = i + 1;

    if (color != search_color)
        return seed_type ^ 1;   /* return segment not of search color */

    return seed_type ^ 0;       /* return segment is of search color */
}


/*
 * ---------------------------------------------------------------------------
 * spans, rectangles & patterned fills
 * ---------------------------------------------------------------------------
 */

/*
 * the fill pattern is a mono bitmap 16 pixels wide, so each pattern word
 * covers 16 consecutive bytes of the framebuffer.
 *
 * note that multifill (a separate pattern per plane) is meaningless for a
 * chunky framebuffer, so it is ignored here.
 */
static void OPTIMIZE_SMALL ch8_fill_rect(const VwkAttrib *attr, const Rect *rect)
{
    const UWORD patmsk = attr->patmsk;
    const int yinc = v_lin_wr;
    UBYTE *addr, *work;
    UBYTE fgcol;
    UWORD mask, pattern;
    int x, y;

    addr = (UBYTE *)get_start_addr(rect->x1, rect->y1);
    fgcol = (UBYTE)attr->color;

    switch(attr->wrt_mode) {
    case WM_ERASE:          /* erase (reverse transparent) mode */
        for (y = rect->y1; y <= rect->y2; y++, addr += yinc) {
            work = addr;
            pattern = attr->patptr[patmsk & y];
            mask = 1U << (15 - (rect->x1 & 0x000f));
            for (x = rect->x1; x <= rect->x2; x++, work++) {
                if (!(pattern & mask))
                    *work = fgcol;
                rorw1(mask);
            }
        }
        break;
    case WM_XOR:            /* xor mode */
        for (y = rect->y1; y <= rect->y2; y++, addr += yinc) {
            work = addr;
            pattern = attr->patptr[patmsk & y];
            mask = 1U << (15 - (rect->x1 & 0x000f));
            for (x = rect->x1; x <= rect->x2; x++, work++) {
                if (pattern & mask)
                    *work = ~*work;
                rorw1(mask);
            }
        }
        break;
    case WM_TRANS:          /* transparent mode */
        for (y = rect->y1; y <= rect->y2; y++, addr += yinc) {
            work = addr;
            pattern = attr->patptr[patmsk & y];
            if (pattern == 0xffff) {        /* common case */
                memset(work, fgcol, rect->x2 - rect->x1 + 1);
                continue;
            }
            mask = 1U << (15 - (rect->x1 & 0x000f));
            for (x = rect->x1; x <= rect->x2; x++, work++) {
                if (pattern & mask)
                    *work = fgcol;
                rorw1(mask);
            }
        }
        break;
    default:                /* replace mode */
        for (y = rect->y1; y <= rect->y2; y++, addr += yinc) {
            work = addr;
            pattern = attr->patptr[patmsk & y];
            if (pattern == 0xffff) {        /* common case */
                memset(work, fgcol, rect->x2 - rect->x1 + 1);
                continue;
            }
            mask = 1U << (15 - (rect->x1 & 0x000f));
            for (x = rect->x1; x <= rect->x2; x++, work++) {
                *work = (pattern & mask) ? fgcol : 0;
                rorw1(mask);
            }
        }
        break;
    }
}


/*
 * ---------------------------------------------------------------------------
 * lines
 * ---------------------------------------------------------------------------
 */

/*
 * the writing mode is tested once per line rather than once per pixel:
 * as in the bitplane and Truecolor drivers, the inner loops matter, so
 * the mode switch is hoisted out of them via this macro.
 *
 * as for the other formats, the background colour for line drawing is
 * always 0: there is no user-settable line background colour.
 */
#define CH8_PIXEL_LOOP(MODE, PLOT, COUNT, STEP)                     \
    case MODE:                                                      \
        for (loopcnt = COUNT; loopcnt >= 0; loopcnt--) {            \
            rolw1(linemask);        /* get next bit of line style */\
            PLOT;                                                   \
            STEP;                                                   \
        }                                                           \
        break

#define CH8_PLOT_ERASE      if (!(linemask&0x0001)) *addr = fgcol
#define CH8_PLOT_XOR        if (linemask&0x0001) *addr = ~*addr
#define CH8_PLOT_TRANS      if (linemask&0x0001) *addr = fgcol
#define CH8_PLOT_REPLACE    *addr = (linemask&0x0001) ? fgcol : 0

/* Bresenham step when x is the major axis, and when y is */
#define CH8_STEP_X                                                  \
    addr++;                                                         \
    eps += e1;                                                      \
    if (eps >= 0) { eps -= e2; addr += yinc; }
#define CH8_STEP_Y                                                  \
    addr += yinc;                                                   \
    eps += e1;                                                      \
    if (eps >= 0) { eps -= e2; addr++; }

static void ch8_draw_line(const Line *line, WORD wrt_mode, UWORD color)
{
    UBYTE *addr;
    UBYTE fgcol;
    UWORD linemask;
    WORD dx, dy, yinc;
    WORD eps, e1, e2;       /* epsilon, epsilon 1, epsilon 2 */
    WORD loopcnt;

    dx = line->x2 - line->x1;
    dy = line->y2 - line->y1;
    yinc = v_lin_wr;

    if (dy < 0) {
        dy = -dy;               /* make dy absolute */
        yinc = -yinc;           /* subtract a line */
    }

    addr = (UBYTE *)get_start_addr(line->x1, line->y1);
    fgcol = (UBYTE)color;
    linemask = LN_MASK;

    if (dx >= dy) {
        e1 = 2*dy;
        eps = -dx;
        e2 = 2*dx;

        switch(wrt_mode) {
        CH8_PIXEL_LOOP(WM_ERASE, CH8_PLOT_ERASE, dx, CH8_STEP_X);
        CH8_PIXEL_LOOP(WM_XOR,   CH8_PLOT_XOR,   dx, CH8_STEP_X);
        CH8_PIXEL_LOOP(WM_TRANS, CH8_PLOT_TRANS, dx, CH8_STEP_X);
        default:                /* WM_REPLACE */
            for (loopcnt = dx; loopcnt >= 0; loopcnt--) {
                rolw1(linemask);
                CH8_PLOT_REPLACE;
                CH8_STEP_X
            }
        }
    } else {
        e1 = 2*dx;
        eps = -dy;
        e2 = 2*dy;

        switch(wrt_mode) {
        CH8_PIXEL_LOOP(WM_ERASE, CH8_PLOT_ERASE, dy, CH8_STEP_Y);
        CH8_PIXEL_LOOP(WM_XOR,   CH8_PLOT_XOR,   dy, CH8_STEP_Y);
        CH8_PIXEL_LOOP(WM_TRANS, CH8_PLOT_TRANS, dy, CH8_STEP_Y);
        default:                /* WM_REPLACE */
            for (loopcnt = dy; loopcnt >= 0; loopcnt--) {
                rolw1(linemask);
                CH8_PLOT_REPLACE;
                CH8_STEP_Y
            }
        }
    }

    LN_MASK = linemask;
}


#if CONF_WITH_VDI_VERTLINE
static void ch8_draw_vertical_line(const Line *line, WORD wrt_mode, UWORD color)
{
    UBYTE *addr;
    UBYTE fgcol;
    UWORD linemask;
    WORD dy, yinc, loopcnt;

    dy = line->y2 - line->y1;
    yinc = v_lin_wr;

    if (dy < 0) {
        dy = -dy;
        yinc = -yinc;
    }

    addr = (UBYTE *)get_start_addr(line->x1, line->y1);
    fgcol = (UBYTE)color;
    linemask = LN_MASK;

    switch(wrt_mode) {
    CH8_PIXEL_LOOP(WM_ERASE, CH8_PLOT_ERASE, dy, addr += yinc);
    CH8_PIXEL_LOOP(WM_XOR,   CH8_PLOT_XOR,   dy, addr += yinc);
    CH8_PIXEL_LOOP(WM_TRANS, CH8_PLOT_TRANS, dy, addr += yinc);
    default:                /* WM_REPLACE */
        for (loopcnt = dy; loopcnt >= 0; loopcnt--, addr += yinc) {
            rolw1(linemask);
            CH8_PLOT_REPLACE;
        }
    }

    LN_MASK = linemask;
}
#endif


/*
 * ---------------------------------------------------------------------------
 * text
 * ---------------------------------------------------------------------------
 */

#if CONF_WITH_VDI_TEXT_SPEEDUP
/*
 * output a character string directly to the screen
 *
 * this is the fast path for a monospaced 8-pixel-wide mono font output
 * byte-aligned with no special effects; see ok_for_direct_blit().
 */
static void ch8_blit_string(WORD count, WORD *str)
{
    WORD height, mode, n;
    WORD src_width, dst_width;
    UBYTE fgcol, mask;
    UBYTE *src, *dst, *save_dst, *p, *q;

    height = DELY;
    mode = WRT_MODE;
    src_width = FWIDTH;
    dst_width = v_lin_wr;
    fgcol = (UBYTE)TEXTFG;

    dst = (UBYTE *)get_start_addr(DESTX, DESTY);

    for ( ; count > 0; count--)
    {
        src = (UBYTE *)FBASE + *str++;
        save_dst = dst;

        for (n = height; n > 0; n--, src += src_width, dst += dst_width)
        {
            p = src;
            q = dst;
            switch(mode) {
            case WM_TRANS:
                for (mask = 0x80; mask; mask >>= 1, q++)
                    if (*p & mask)
                        *q = fgcol;
                break;
            case WM_XOR:
                for (mask = 0x80; mask; mask >>= 1, q++)
                    if (*p & mask)
                        *q = ~*q;
                break;
            case WM_ERASE:
                for (mask = 0x80; mask; mask >>= 1, q++)
                    if (!(*p & mask))
                        *q = fgcol;
                break;
            default:            /* WM_REPLACE */
                for (mask = 0x80; mask; mask >>= 1, q++)
                    *q = (*p & mask) ? fgcol : 0;
            }
        }

        dst = save_dst + 8;     /* next character cell */
    }
}
#endif


/*
 * output a glyph to the screen
 *
 * this is the general path: the source is a mono bitmap (either the font
 * itself or the intermediate buffer produced by rotation/scaling/
 * outlining), starting at the bottom line and working upwards.
 */
static void ch8_blit_glyph(LOCALVARS *vars)
{
    UBYTE *src, *dst, *p, *q;
    UBYTE fgcol;
    UWORD src_mask, mask, skew_mask;
    WORD h, w, skew, skew_start;
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
     * set skew-related values
     *
     * NOTE: we can't test for skewed text using vars->STYLE, since
     * pre_blit() clears F_SKEW and F_THICKEN after it has processed them.
     */
    skew = LOFF + ROFF;
    skew_mask = (UWORD)vars->skew_msk;
    skew_start = vars->height;

    /* see the equivalent code in vdi_raster_truecolor.c for the rationale */
    if (skew && (vars->STYLE&F_OUTLINE))
    {
        if (SOURCEX)
        {
            SOURCEX = 0;
            vars->tsdad = 0;
        }

        if (DESTX < 0)
            vars->DESTX = DESTX;

        if (vars->height > 8)       /* not a 6-point font */
            skew_start -= OUTLINE_THICKNESS;
    }

    src = vars->sform;
    src_mask = 0x8000 >> vars->tsdad;

    vars->dform = v_bas_ad;
    vars->dform += vars->DESTX;     /* one byte per pixel */
    vars->dform += (UWORD)(vars->DESTY+vars->DELY-1) * (ULONG)v_lin_wr;
    vars->d_next = -v_lin_wr;
    dst = vars->dform;

    fgcol = (UBYTE)vars->forecol;

    /*
     * one loop per writing mode, so that the mode is not retested for
     * every pixel of every glyph.
     *
     * the skew handling at the end of each row is explained in the
     * equivalent Truecolor code: since skewed character cells are
     * effectively slanted, the starting position of a cell must shift
     * rightwards as we go up the character.
     */
#define CH8_GLYPH_LOOP(PLOT)                                                \
    for (h = vars->height; h > 0; h--, src += vars->s_next, dst += vars->d_next) \
    {                                                                       \
        p = src;                                                            \
        q = dst;                                                            \
        for (w = vars->width, mask = src_mask; w > 0; w--, q++)             \
        {                                                                   \
            PLOT;                                                           \
            rorw1(mask);                                                    \
            if (mask == 0x8000)                                             \
                p += sizeof(UWORD);                                         \
        }                                                                   \
        if (skew && (h <= skew_start))                                      \
        {                                                                   \
            rolw1(skew_mask);                                               \
            if (skew_mask & 0x8000)                                         \
            {                                                               \
                rorw1(src_mask);                                            \
                if (src_mask == 0x8000)                                     \
                    src += sizeof(UWORD);                                   \
                dst++;                                                      \
            }                                                               \
        }                                                                   \
    }

    switch(vars->WRT_MODE) {
    case WM_TRANS:
        CH8_GLYPH_LOOP(if (*(UWORD *)p & mask) *q = fgcol)
        break;
    case WM_XOR:
        CH8_GLYPH_LOOP(if (*(UWORD *)p & mask) *q = ~*q)
        break;
    case WM_ERASE:
        CH8_GLYPH_LOOP(if (!(*(UWORD *)p & mask)) *q = fgcol)
        break;
    default:                /* WM_REPLACE */
        CH8_GLYPH_LOOP(*q = (*(UWORD *)p & mask) ? fgcol : 0)
    }
#undef CH8_GLYPH_LOOP
}


/*
 * ---------------------------------------------------------------------------
 * misc
 * ---------------------------------------------------------------------------
 */

static void ch8_clear_screen(void)
{
    memset(v_bas_ad, 0x00, (ULONG)v_lin_wr * V_REZ_VT);
}


/*
 * The raster-copy entries are still the planar ones.  A chunky screen is
 * eight interleaved planes seen sideways, so bit_blt() moves the right
 * bytes for a screen-to-screen copy in the common (byte-aligned, replace
 * mode) case, and getting the general case right needs a chunky blit
 * engine of its own.  Everything the desktop draws itself now goes through
 * the chunky entries above; this is the remaining piece of work.
 */
const VDI_RASTER_DRIVER vdi_raster_chunky8 = {
    .name = "Foenix chunky 8bpp",
    .resolution_changed = NULL,
    .get_pixel = ch8_get_pixel,
    .put_pixel = ch8_put_pixel,
    .fill_rect = ch8_fill_rect,
    .draw_line = ch8_draw_line,
#if CONF_WITH_VDI_VERTLINE
    .draw_vertical_line = ch8_draw_vertical_line,
#endif
    .setup_forms = vdi_setup_forms_planar,
    .copy_raster_opaque = vdi_copy_raster_opaque_planar,
    .copy_raster_transparent = vdi_copy_raster_transparent_planar,
    .blit = blit_frame_copy,
    .transform_form = vdi_transform_form_planar,
    .find_span = ch8_find_span,
    .blit_glyph = ch8_blit_glyph,
#if CONF_WITH_VDI_TEXT_SPEEDUP
    .blit_string = ch8_blit_string,
#endif
    .clear_screen = ch8_clear_screen,
};

#endif /* CONF_WITH_CHUNKY8 */
