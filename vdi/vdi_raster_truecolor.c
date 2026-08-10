/*
 * vdi_raster_truecolor.c - VDI screen raster driver for packed 16-bit pixels
 *
 * This is the Falcon Truecolor implementation of VDI_RASTER_DRIVER: each
 * pixel is a single 16-bit word holding an RGB value, so there are no
 * bitplanes to walk.  See vdi_raster_driver.h for the interface, and
 * vdi_raster_bitplane.c for the Atari interleaved-bitplane driver.
 *
 * The bodies below were previously the "*16" branches of the shared VDI
 * raster code, selected at each call site by testing TRUECOLOR_MODE.
 *
 * Copyright 1999 by Caldera, Inc. and Authors:
 * Copyright 2002-2025 The EmuTOS development team
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

/* #define ENABLE_KDEBUG */

#include "emutos.h"

#if CONF_WITH_VDI_16BIT

#include "asm.h"
#include "intmath.h"
#include "aesext.h"
#include "string.h"     /* for memset(), bzero() */
#include "gemdos.h"     /* for mem alloc & free */
#include "tosvars.h"
#include "lineavars.h"
#include "vdi_defs.h"
#include "vdistub.h"
#include "vdi_blit.h"
#include "vdi_inline.h"
#include "vdi_textblit.h"
#include "vdi_raster_driver.h"
#include "../bios/videl.h"      /* for OVERLAY_BIT */

/*
 * ---------------- pixel probing (flood fill) ----------------
 */

/*
 * search_to_right16() - Truecolor version of search_to_right()
 */
static UWORD search_to_right16(const VwkClip *clip, WORD x, const UWORD search_col, UWORD *addr)
{
    UWORD pixel, search;

    search = search_col & ~OVERLAY_BIT; /* ignore overlay bit in search colour */

    /*
     * scan upwards until pixel of different colour found
     */
    for ( ; x <= clip->xmx_clip; x++)
    {
        pixel = *addr++ & ~OVERLAY_BIT; /* ignore overlay bit on screen */
        if (pixel != search)
            break;
    }

    return x - 1;
}



/*
 * search_to_left16() - Truecolor version of search_to_left()
 */
static UWORD search_to_left16(const VwkClip *clip, WORD x, const UWORD search_col, UWORD *addr)
{
    UWORD pixel, search;

    search = search_col & ~OVERLAY_BIT; /* ignore overlay bit in search colour */

    /*
     * scan downwards until pixel of different colour found
     */
    for ( ; x >= clip->xmn_clip; x--)
    {
        pixel = *addr-- & ~OVERLAY_BIT; /* ignore overlay bit on screen */
        if (pixel != search)
            break;
    }

    return x + 1;
}



/*
 * end_pts16() - Truecolor version of end_pts()
 */
static WORD end_pts16(const VwkClip *clip, WORD x, WORD y, UWORD search_color, BOOL seed_type, WORD *xleftout, WORD *xrightout)
{
    UWORD color;
    UWORD *addr;

    /*
     * convert x,y to start address and get colour
     */
    addr = get_start_addr16(x, y);
    color = *addr & ~OVERLAY_BIT;    /* ignore overlay bit on screen */

    /*
     * get left and right end
     */
    *xrightout = search_to_right16(clip, x, color, addr);
    *xleftout = search_to_left16(clip, x, color, addr);

    if (color != search_color)
        return seed_type ^ 1;   /* return segment not of search color */

    return seed_type ^ 0;       /* return segment is of search color */
}


/*
 * swblit_rect_common16 - draw one or more horizontal lines via software, 16-bit mode
 *
 * this is much simpler than the corresponding bitplane code
 * (see swblit_rect_common() in vdi_raster_bitplane_line.c)
 *
 * FIXME: notes to self
 * . do we need OPTIMIZE_SMALL?
 * . pay attention to notes in Compendium for vswr_mode re ERASE & TRANS
 */
static void OPTIMIZE_SMALL swblit_rect_common16(const VwkAttrib *attr, const Rect *rect)
{
    const UWORD patmsk = attr->patmsk;
    const int yinc = v_lin_wr >> 1;     /* in WORDs */
    UWORD *addr, *work, *palette;
    UWORD bgcol, fgcol, mask, pattern;
    int x, y, patind;

    addr = get_start_addr16(rect->x1, rect->y1);
    palette = CUR_WORK->ext->palette;
    fgcol = palette[attr->color];
    bgcol = palette[0];     //FIXME ??

    switch(attr->wrt_mode) {
    case WM_ERASE:          /* erase (reverse transparent) mode */
        for (y = rect->y1; y <= rect->y2; y++, addr += yinc) {
            work = addr;
            patind = patmsk & y;            /* starting pattern index */
            pattern = attr->patptr[patind];
            if (pattern == 0x0000) {        /* simple case */
                for (x = rect->x1; x <= rect->x2; x++) {
                    *work++ = fgcol;
                }
            } else {
                mask = 1U << (15 - (rect->x1 & 0x000f));
                for (x = rect->x1; x <= rect->x2; x++, work++) {
                    if (!(pattern & mask)) {
                        *work = fgcol;
                    }
                    rorw1(mask);
                }
            }
        }
        break;
    case WM_XOR:            /* xor mode */
        for (y = rect->y1; y <= rect->y2; y++, addr += yinc) {
            UWORD temp;

            work = addr;
            patind = patmsk & y;            /* starting pattern index */
            pattern = attr->patptr[patind];
            if (pattern == 0xffff) {        /* common case */
                for (x = rect->x1; x <= rect->x2; x++) {
                    temp = *work;
                    *work++ = ~temp;        /* complement the existing colour */
                }
            } else {
                mask = 1U << (15 - (rect->x1 & 0x000f));
                for (x = rect->x1; x <= rect->x2; x++, work++) {
                    if (pattern & mask) {
                        temp = *work;
                        *work = ~temp;      /* complement the existing colour */
                    }
                    rorw1(mask);
                }
            }
        }
        break;
    case WM_TRANS:          /* transparent mode */
        for (y = rect->y1; y <= rect->y2; y++, addr += yinc) {
            work = addr;
            patind = patmsk & y;            /* starting pattern index */
            pattern = attr->patptr[patind];
            if (pattern == 0xffff) {        /* common case */
                for (x = rect->x1; x <= rect->x2; x++) {
                    *work++ = fgcol;
                }
            } else {
                mask = 1U << (15 - (rect->x1 & 0x000f));
                for (x = rect->x1; x <= rect->x2; x++, work++) {
                    if (pattern & mask) {
                        *work = fgcol;
                    }
                    rorw1(mask);
                }
            }
        }
        break;
    default:                /* replace mode */
        for (y = rect->y1; y <= rect->y2; y++, addr += yinc) {
            work = addr;
            patind = patmsk & y;            /* starting pattern index */
            pattern = attr->patptr[patind];
            if (pattern == 0xffff) {        /* common case */
                for (x = rect->x1; x <= rect->x2; x++) {
                    *work++ = fgcol;
                }
            } else {
                mask = 1U << (15 - (rect->x1 & 0x000f));
                for (x = rect->x1; x <= rect->x2; x++) {
                    if (pattern & mask) {
                        *work++ = fgcol;
                    } else {
                        *work++ = bgcol;
                    }
                    rorw1(mask);
                }
            }
        }
        break;
    }
}


/*
 * draw_line16 - draw a line (general purpose) in 16-bit graphics
 *
 * see draw_line() in vdi_raster_bitplane_line.c for further info
 */
static void draw_line16(const Line *line, WORD wrt_mode, UWORD color)
{
    UWORD *addr, *palette;
    UWORD bgcol, fgcol, linemask;
    WORD dx, dy, yinc;
    WORD eps, e1, e2;       /* epsilon, epsilon 1, epsilon 2 */
    WORD loopcnt;

    dx = line->x2 - line->x1;
    dy = line->y2 - line->y1;
    yinc = v_lin_wr / 2;        /* in words */

    if (dy < 0) {
        dy = -dy;               /* make dy absolute */
        yinc = -yinc;           /* subtract a line */
    }

    addr = get_start_addr16(line->x1, line->y1);    /* init address counter */
    palette = CUR_WORK->ext->palette;
    fgcol = palette[color];
    bgcol = palette[0];

    linemask = LN_MASK;

    if (dx >= dy) {
        e1 = 2*dy;
        eps = -dx;
        e2 = 2*dx;

        switch(wrt_mode) {
        case WM_ERASE:      /* reverse transparent  */
            for (loopcnt = dx; loopcnt >= 0; loopcnt--) {
                rolw1(linemask);        /* get next bit of line style */
                if (!(linemask&0x0001))
                    *addr = fgcol;
                addr++;
                eps += e1;
                if (eps >= 0 ) {
                    eps -= e2;
                    addr += yinc;       /* increment y */
                }
            }
            break;
        case WM_XOR:        /* xor */
            for (loopcnt = dx; loopcnt >= 0; loopcnt--) {
                rolw1(linemask);        /* get next bit of line style */
                if (linemask&0x0001)
                    *addr = ~*addr;
                addr++;
                eps += e1;
                if (eps >= 0 ) {
                    eps -= e2;
                    addr += yinc;       /* increment y */
                }
            }
            break;
        case WM_TRANS:      /* transparent */
            for (loopcnt = dx; loopcnt >= 0; loopcnt--) {
                rolw1(linemask);        /* get next bit of line style */
                if (linemask&0x0001)
                    *addr = fgcol;
                addr++;
                eps += e1;
                if (eps >= 0 ) {
                    eps -= e2;
                    addr += yinc;       /* increment y */
                }
            }
            break;
        case WM_REPLACE:    /* replace */
            for (loopcnt = dx; loopcnt >= 0; loopcnt--) {
                rolw1(linemask);        /* get next bit of line style */
                if (linemask&0x0001)
                    *addr = fgcol;
                else
                    *addr = bgcol;
                addr++;
                eps += e1;
                if (eps >= 0 ) {
                    eps -= e2;
                    addr += yinc;       /* increment y */
                }
            }
        }
    } else {        /* dx < dy */
        e1 = 2*dx;
        eps = -dy;
        e2 = 2*dy;

        switch(wrt_mode) {
        case WM_ERASE:      /* reverse transparent */
            for (loopcnt = dy; loopcnt >= 0; loopcnt--) {
                rolw1(linemask);        /* get next bit of line style */
                if (!(linemask&0x0001))
                    *addr = fgcol;
                addr += yinc;
                eps += e1;
                if (eps >= 0 ) {
                    eps -= e2;
                    addr++;
                }
            }
            break;
        case WM_XOR:        /* xor */
            for (loopcnt = dy; loopcnt >= 0; loopcnt--) {
                rolw1(linemask);        /* get next bit of line style */
                if (linemask&0x0001)
                    *addr = ~*addr;
                addr += yinc;
                eps += e1;
                if (eps >= 0 ) {
                    eps -= e2;
                    addr++;
                }
            }
            break;
        case WM_TRANS:      /* transparent */
            for (loopcnt = dy; loopcnt >= 0; loopcnt--) {
                rolw1(linemask);        /* get next bit of line style */
                if (linemask&0x0001)
                    *addr = fgcol;
                addr += yinc;
                eps += e1;
                if (eps >= 0 ) {
                    eps -= e2;
                    addr++;
                }
            }
            break;
        case WM_REPLACE:    /* replace */
            for (loopcnt = dy; loopcnt >= 0; loopcnt--) {
                rolw1(linemask);        /* get next bit of line style */
                if (linemask&0x0001)
                    *addr = fgcol;
                else
                    *addr = bgcol;
                addr += yinc;
                eps += e1;
                if (eps >= 0 ) {
                    eps -= e2;
                    addr++;
                }
            }
        }
    }

    LN_MASK = linemask;
}


#if CONF_WITH_VDI_VERTLINE
/*
 * swblit_vertical_line16 - draw a vertical line in 16-bit graphics
 */
static void swblit_vertical_line16(const Line *line, WORD wrt_mode, UWORD color)
{
    UWORD *addr, *palette;
    WORD dy;                    /* length of line */
    WORD yinc;                  /* in/decrease for each y step */
    WORD loopcnt;
    UWORD bgcol, fgcol, linemask;

    /* calculate increase value for y to add to actual address */
    dy = line->y2 - line->y1;
    yinc = v_lin_wr / 2;        /* one line of words */

    if (dy < 0) {
        dy = -dy;               /* make dy absolute */
        yinc = -yinc;           /* sub one line of words */
    }

    addr = get_start_addr16(line->x1, line->y1);    /* init address counter */
    palette = CUR_WORK->ext->palette;
    fgcol = palette[color];
    bgcol = palette[0];     //FIXME ??

    linemask = LN_MASK;

    switch(wrt_mode) {
    case WM_ERASE:          /* reverse transparent */
        for (loopcnt = dy; loopcnt >= 0; loopcnt--) {
            rolw1(linemask);        /* get next bit of line style */
            if (!(linemask & 0x0001))
                *addr = fgcol;
            addr += yinc;
        }
        break;
    case WM_XOR:            /* xor */
        for (loopcnt = dy; loopcnt >= 0; loopcnt--) {
            rolw1(linemask);        /* get next bit of line style */
            if (linemask & 0x0001)
                *addr = ~*addr;
            addr += yinc;
        }
        break;
    case WM_TRANS:          /* transparent */
        for (loopcnt = dy; loopcnt >= 0; loopcnt--) {
            rolw1(linemask);        /* get next bit of line style */
            if (linemask & 0x0001)
                *addr = fgcol;
            addr += yinc;
        }
        break;
    case WM_REPLACE:        /* replace */
        for (loopcnt = dy; loopcnt >= 0; loopcnt--) {
            rolw1(linemask);        /* get next bit of line style */
            if (linemask & 0x0001)
                *addr = fgcol;
            else
                *addr = bgcol;
            addr += yinc;
        }
    }

    LN_MASK = linemask;
}
#endif


/*
 * convert between 16-bit standard format and Truecolor device-dependent format
 */
static void vr_trnfm16(MFDB *src_mfdb, MFDB *dst_mfdb)
{
    WORD *src, *dst, *work, *tempbuf = NULL;
    LONG i, planesize, formsize;
    UWORD src_mask, dst_mask;

    src = src_mfdb->fd_addr;
    dst = dst_mfdb->fd_addr;
    planesize = (LONG)src_mfdb->fd_h * src_mfdb->fd_wdwidth;    /* in words */
    formsize = planesize * sizeof(WORD) * 16;   /* in bytes */

    /*
     * for now, if 'in place' we actually do a normal transform to a temp buf.
     * the only problem with this is that we fail silently if memory is tight.
     */
    if (src == dst)
    {
        tempbuf = dos_alloc_anyram(formsize);
        if (!tempbuf)
        {
            KDEBUG(("Cannot allocate temp buf for vr_trnfm()\n"));
            return;
        }
        dst = tempbuf;
    }

    bzero(dst, formsize);   /* clear out the output */

    if (src_mfdb->fd_stand) /* handle standard -> device-dependent */
    {
        for (dst_mask = 0x8000, work = dst; dst_mask; dst_mask >>= 1, work = dst)
        {
            for (i = 0; i < planesize; i++, src++)
            {
                for (src_mask = 0x8000; src_mask; src_mask >>= 1, work++)
                {
                    if (*src & src_mask)
                    {
                        *work |= dst_mask;
                    }
                }
            }
        }
    }
    else                    /* handle device-dependent -> standard */
    {
        for (src_mask = 0x8000, work = src; src_mask; src_mask >>= 1, work = src)
        {
            for (i = 0; i < planesize; i++, dst++)
            {
                for (dst_mask = 0x8000; dst_mask; dst_mask >>= 1, work++)
                {
                    if (*work & src_mask)
                    {
                        *dst |= dst_mask;
                    }
                }
            }
        }
    }

    if (tempbuf)
    {
        memcpy(dst_mfdb->fd_addr, tempbuf, formsize);
        dos_free(tempbuf);
    }
}


/*
 * vro_cpyfm16() - handle vro_cpyfm() for 16-bit graphics
 *
 * the logic ops all act literally on the pixels (words) concerned
 */
void vro_cpyfm16(struct blit_frame *info)
{
    UWORD *src, *dst, *p, *q;
    WORD src_width, dst_width, next_pixel;
    WORD mode, rows, cols;

    mode = INTIN[0];

    /*
     * init pointers & increments
     */
    src_width = info->s_nxln / sizeof(WORD);
    dst_width = info->d_nxln / sizeof(WORD);
    next_pixel = 1;
    src = info->s_form + ((LONG)info->s_ymin * src_width) + info->s_xmin;
    dst = info->d_form + ((LONG)info->d_ymin * dst_width) + info->d_xmin;

    /*
     * adjust if potential overlap
     */
    if (src < dst) {
        src = info->s_form + ((LONG)info->s_ymax * src_width) + info->s_xmax;
        dst = info->d_form + ((LONG)info->d_ymax * dst_width) + info->d_xmax;
        src_width = -src_width;
        dst_width = -dst_width;
        next_pixel = -1;
    }

    p = src;
    q = dst;

    rows = info->s_ymax - info->s_ymin + 1;

    switch(mode) {
    case BM_ALL_WHITE:  /* D1 = 0 */
        while(rows-- > 0) {
            cols = info->s_xmax - info->s_xmin + 1;
            while(cols-- > 0) {
                *q = 0;
                q += next_pixel;
            }
            dst += dst_width;
            q = dst;
        }
        break;
    case BM_S_AND_D:    /* D1 = S AND D */
        while(rows-- > 0) {
            cols = info->s_xmax - info->s_xmin + 1;
            while(cols-- > 0) {
                *q = *p & *q;
                p += next_pixel;
                q += next_pixel;
            }
            src += src_width;
            p = src;
            dst += dst_width;
            q = dst;
        }
        break;
    case BM_S_AND_NOTD: /* D1 = S AND (NOT D) */
        while(rows-- > 0) {
            cols = info->s_xmax - info->s_xmin + 1;
            while(cols-- > 0) {
                *q = *p & ~*q;
                p += next_pixel;
                q += next_pixel;
            }
            src += src_width;
            p = src;
            dst += dst_width;
            q = dst;
        }
        break;
    case BM_S_ONLY:     /* D1 = S */                /* replace */
        /*
         * this is the common case, so we optimize a bit
         */
        while(rows-- > 0) {
            cols = info->s_xmax - info->s_xmin + 1;
            if (src < dst) {
                while(cols-- > 0) {
                    *q-- = *p--;
                }
            } else {
                while(cols-- > 0) {
                    *q++ = *p++;
                }
            }
            src += src_width;
            p = src;
            dst += dst_width;
            q = dst;
        }
        break;
    case BM_NOTS_AND_D: /* D1 = (NOT S) AND D */    /* erase */
        while(rows-- > 0) {
            cols = info->s_xmax - info->s_xmin + 1;
            while(cols-- > 0) {
                *q = ~*p & *q;
                p += next_pixel;
                q += next_pixel;
            }
            src += src_width;
            p = src;
            dst += dst_width;
            q = dst;
        }
        break;
    case BM_D_ONLY:     /* D1 = D */
        /* nothing to do */
        break;
    case BM_S_XOR_D:    /* D1 = S XOR D */          /* XOR */
        while(rows-- > 0) {
            cols = info->s_xmax - info->s_xmin + 1;
            while(cols-- > 0) {
                *q = *p ^ *q;
                p += next_pixel;
                q += next_pixel;
            }
            src += src_width;
            p = src;
            dst += dst_width;
            q = dst;
        }
        break;
    case BM_S_OR_D:     /* D1 = S OR D */           /* transparent */
        while(rows-- > 0) {
            cols = info->s_xmax - info->s_xmin + 1;
            while(cols-- > 0) {
                *q = *p | *q;
                p += next_pixel;
                q += next_pixel;
            }
            src += src_width;
            p = src;
            dst += dst_width;
            q = dst;
        }
        break;
    case BM_NOT_SORD:   /* D1 = NOT (S OR D) */
        while(rows-- > 0) {
            cols = info->s_xmax - info->s_xmin + 1;
            while(cols-- > 0) {
                *q = ~(*p | *q);
                p += next_pixel;
                q += next_pixel;
            }
            src += src_width;
            p = src;
            dst += dst_width;
            q = dst;
        }
        break;
    case BM_NOT_SXORD:  /* D1 = NOT (S XOR D) */
        while(rows-- > 0) {
            cols = info->s_xmax - info->s_xmin + 1;
            while(cols-- > 0) {
                *q = ~(*p ^ *q);
                p += next_pixel;
                q += next_pixel;
            }
            src += src_width;
            p = src;
            dst += dst_width;
            q = dst;
        }
        break;
    case BM_NOT_D:      /* D1 = NOT D */
        while(rows-- > 0) {
            cols = info->s_xmax - info->s_xmin + 1;
            while(cols-- > 0) {
                *q = ~*q;
                q += next_pixel;
            }
            dst += dst_width;
            q = dst;
        }
        break;
    case BM_S_OR_NOTD:  /* D1 = S OR (NOT D) */
        while(rows-- > 0) {
            cols = info->s_xmax - info->s_xmin + 1;
            while(cols-- > 0) {
                *q = *p | ~*q;
                p += next_pixel;
                q += next_pixel;
            }
            src += src_width;
            p = src;
            dst += dst_width;
            q = dst;
        }
        break;
    case BM_NOT_S:      /* D1 = NOT S */
        while(rows-- > 0) {
            cols = info->s_xmax - info->s_xmin + 1;
            while(cols-- > 0) {
                *q = ~*p;
                p += next_pixel;
                q += next_pixel;
            }
            src += src_width;
            p = src;
            dst += dst_width;
            q = dst;
        }
        break;
    case BM_NOTS_OR_D:  /* D1 = (NOT S) OR D */     /* reverse transparent */
        while(rows-- > 0) {
            cols = info->s_xmax - info->s_xmin + 1;
            while(cols-- > 0) {
                *q = ~*p | *q;
                p += next_pixel;
                q += next_pixel;
            }
            src += src_width;
            p = src;
            dst += dst_width;
            q = dst;
        }
        break;
    case BM_NOT_SANDD:  /* D1 = NOT (S AND D) */
        while(rows-- > 0) {
            cols = info->s_xmax - info->s_xmin + 1;
            while(cols-- > 0) {
                *q = ~(*p & *q);
                p += next_pixel;
                q += next_pixel;
            }
            src += src_width;
            p = src;
            dst += dst_width;
            q = dst;
        }
        break;
    case BM_ALL_BLACK:  /* D1 = 1 */
        while(rows-- > 0) {
            cols = info->s_xmax - info->s_xmin + 1;
            while(cols-- > 0) {
                *q = 0xffff;
                q += next_pixel;
            }
            dst += dst_width;
            q = dst;
        }
        break;
    }
}

/*
 * vrt_cpyfm16() - handle vrt_cpyfm() for 16-bit graphics
 */
void vrt_cpyfm16(struct blit_frame *info)
{
    UWORD *src, *dst, *p, *q;
    WORD mode, src_width, src_off, dst_width, x, y;
    UWORD *palette, src_mask, bit_mask, fgcol, bgcol;

    mode = INTIN[0];

    palette = CUR_WORK->ext->palette;
    fgcol = palette[info->fg_col];
    bgcol = palette[info->bg_col];

    /*
     * init source area variables
     */
    src_width = info->s_nxln / sizeof(WORD);/* width in words */
    src_off = info->s_xmin >> 4;            /* starting x offset in words */
    bit_mask = src_mask = 0x8000U >> (info->s_xmin&0x000f); /* starting bit mask */
    p = src = info->s_form + ((LONG)info->s_ymin * src_width) + src_off;

    /*
     * init destination area variables
     */
    dst_width = info->d_nxln / sizeof(WORD);    /* in words */
    q = dst = info->d_form + ((LONG)info->d_ymin * dst_width) + info->d_xmin;

    switch(mode) {
    case MD_ERASE:
        for (y = info->s_ymin; y <= info->s_ymax; y++) {
            for (x = info->s_xmin; x <= info->s_xmax; x++, q++) {
                if (!(*p & bit_mask))
                    *q = bgcol;
                rorw1(bit_mask);
                if (bit_mask & 0x8000)
                    p++;
            }
            src += src_width;
            p = src;
            bit_mask = src_mask;
            dst += dst_width;
            q = dst;
        }
        break;

    case MD_XOR:
        for (y = info->s_ymin; y <= info->s_ymax; y++) {
            for (x = info->s_xmin; x <= info->s_xmax; x++, q++) {
                if (*p & bit_mask)
                    *q = ~*q;
                rorw1(bit_mask);
                if (bit_mask & 0x8000)
                    p++;
            }
            src += src_width;
            p = src;
            bit_mask = src_mask;
            dst += dst_width;
            q = dst;
        }
        break;

    case MD_TRANS:
        for (y = info->s_ymin; y <= info->s_ymax; y++) {
            for (x = info->s_xmin; x <= info->s_xmax; x++, q++) {
                if (*p & bit_mask)
                    *q = fgcol;
                rorw1(bit_mask);
                if (bit_mask & 0x8000)
                    p++;
            }
            src += src_width;
            p = src;
            bit_mask = src_mask;
            dst += dst_width;
            q = dst;
        }
        break;

    case MD_REPLACE:
        for (y = info->s_ymin; y <= info->s_ymax; y++) {
            for (x = info->s_xmin; x <= info->s_xmax; x++, q++) {
                if (*p & bit_mask)
                    *q = fgcol;
                else
                    *q = bgcol;
                rorw1(bit_mask);
                if (bit_mask & 0x8000)
                    p++;
            }
            src += src_width;
            p = src;
            bit_mask = src_mask;
            dst += dst_width;
            q = dst;
        }
        break;

    default:
        return;                     /* unsupported mode */
    }
}


#if CONF_WITH_VDI_TEXT_SPEEDUP
/*
 * output a character string directly to the 16-bit screen
 *
 * this is the fast path for a monospaced 8-pixel-wide mono font output
 * byte-aligned with no special effects; see ok_for_direct_blit()
 */
static void direct_screen_blit16(WORD count, WORD *str)
{
    WORD fgcol, bgcol, height, mode, n;
    WORD src_width, dst_width;
    UBYTE mask;
    UBYTE *src, *p;
    UWORD *dst, *save_dst, *q, *palette;

    height = DELY;
    mode = WRT_MODE;
    src_width = FWIDTH;
    dst_width = v_lin_wr / sizeof(UWORD);

    palette = CUR_WORK->ext->palette;
    fgcol = palette[TEXTFG];
    bgcol = palette[0];

    dst = get_start_addr16(DESTX, DESTY);

    for ( ; count > 0; count--)
    {
        src = (UBYTE *)FBASE + *str++;
        save_dst = dst;

        switch(mode) {
        default:    /* WM_REPLACE */
            for (n = height, p = src; n > 0; n--)
            {
                for (mask = 0x80, q = dst; mask; mask >>= 1)
                {
                    *q++ = (*p & mask) ? fgcol : bgcol;
                }
                p += src_width;
                dst += dst_width;
            }
            break;
        case WM_TRANS:
            for (n = height, p = src; n > 0; n--)
            {
                for (mask = 0x80, q = dst; mask; mask >>= 1)
                {
                    if (*p & mask)
                        *q = fgcol;
                    q++;
                }
                p += src_width;
                dst += dst_width;
            }
            break;
        case WM_XOR:
            for (n = height, p = src; n > 0; n--)
            {
                for (mask = 0x80, q = dst; mask; mask >>= 1)
                {
                    if (*p & mask)
                        *q = ~*q;
                    q++;
                }
                p += src_width;
                dst += dst_width;
            }
            break;
        case WM_ERASE:
            for (n = height, p = src; n > 0; n--)
            {
                for (mask = 0x80, q = dst; mask; mask >>= 1)
                {
                    /*
                     * note: here we differ from TOS 4.04 which seems to
                     * behave as though the assignment below was "*q = bgcol;".
                     * the TOS4.04 behaviour is a bug IMO.
                     */
                    if (!(*p & mask))
                        *q = fgcol;
                    q++;
                }
                p += src_width;
                dst += dst_width;
            }
            break;
        }

        dst = save_dst + 8;
    }
}
#endif


/*
 * output a glyph to the 16-bit screen
 */
static void screen_blit16(LOCALVARS *vars)
{
    UWORD *palette, *p, *q;
    UBYTE *src, *dst;
    WORD fgcol, bgcol, h, w, skew, skew_start;
    UWORD src_mask, mask, skew_mask;

    /*
     * set skew-related values
     *
     * NOTE: we can't test for skewed text using vars->STYLE, since
     * pre_blit() clears F_SKEW and F_THICKEN after it has processed them.
     */
    skew = LOFF + ROFF;
    skew_mask = (UWORD)vars->skew_msk;
    skew_start = vars->height;

    /*
     * the following adjustments are for skewed+outlined text, and make
     * the output almost the same as produced by TOS4.
     *
     * 1. since the source of skewed and/or outlined text must be an
     *    intermediate buffer, SOURCEX *must* be 0, and we force that.
     *    NOTE: in versions of TOS prior to TOS4 (& in TOS4 non-TC
     *    resolutions), this adjustment is not made.  As a result, text
     *    output is typically clipped.
     *
     * 2. a negative value for the nominal destination position is OK,
     *    because outlining has adjusted the starting position of characters
     *    leftwards.  however, such values are prohibited by do_clip(),
     *    which adjusts var->DESTX.  we adjust it back here ...
     *    NOTE: this situation can only happen at the beginning of a
     *    screen line.
     *
     * 3. for bigger fonts, skewing must not start at the bottom of the
     *    buffer, otherwise parts of the outline are clipped too agressively.
     *    at the moment, this fix is a bit of a kludge, though it works well
     *    enough.
     */
    if (skew && (vars->STYLE&F_OUTLINE))
    {
        if (SOURCEX)
        {
            KDEBUG(("SOURCEX (was %d) forced to zero for intermediate buffer\n",SOURCEX));
            SOURCEX = 0;
            vars->tsdad = 0;    /* this was set from SOURCEX in screen_blit() */
        }

        if (DESTX < 0)
        {
            KDEBUG(("vars->DESTX (was %d) set to DESTX (%d)\n",vars->DESTX,DESTX));
            vars->DESTX = DESTX;
        }
        if (vars->height > 8)       /* not a 6-point font */
            skew_start -= OUTLINE_THICKNESS;
    }

    /*
     * set up source stuff
     */
    src = vars->sform;
    src_mask = 0x8000 >> vars->tsdad;

    /*
     * set up destination stuff
     */
    vars->dform = v_bas_ad;
    vars->dform += vars->DESTX * sizeof(WORD);      /* add x coordinate part of addr */
    vars->dform += (UWORD)(vars->DESTY+vars->DELY-1) * (ULONG)v_lin_wr; /* add y coordinate part of addr */
    vars->d_next = -v_lin_wr;
    dst = vars->dform;

    /*
     * set up colours
     */
    palette = CUR_WORK->ext->palette;
    fgcol = palette[vars->forecol];
    bgcol = palette[0];

    switch(vars->WRT_MODE) {
    /*
     * when called via lineA, modes 4-19 (corresponding to BitBlt modes 0-15)
     * are theoretically possible.  however, at this time we do not support them.
     */
    default:    /* WM_REPLACE */
        for (h = vars->height; h > 0; h--, src += vars->s_next, dst += vars->d_next)
        {
            p = (UWORD *)src;
            q = (UWORD *)dst;
            for (w = vars->width, mask = src_mask; w > 0; w--)
            {
                *q++ = (*p & mask) ? fgcol : bgcol;
                rorw1(mask);
                if (mask == 0x8000)
                    p++;
            }
            /*
             * special handling for skewed text: since the character cells
             * are effectively slanted, we must shift the starting position
             * of a cell rightwards as we go up the character.
             */
            if (skew && (h <= skew_start))  /* OK to shift box for skewed text? */
            {
                rolw1(skew_mask);
                if (skew_mask & 0x8000)
                {
                    rorw1(src_mask);
                    if (src_mask == 0x8000)
                        src++;
                    dst += sizeof(UWORD);
                }
            }
        }
        break;
    case WM_TRANS:
        for (h = vars->height; h > 0; h--, src += vars->s_next, dst += vars->d_next)
        {
            p = (UWORD *)src;
            q = (UWORD *)dst;
            for (w = vars->width, mask = src_mask; w > 0; w--)
            {
                if (*p & mask)
                    *q = fgcol;
                q++;
                rorw1(mask);
                if (mask == 0x8000)
                    p++;
            }
            /*
             * see comments for WM_REPLACE (above) for an explanation of
             * the following
             */
            if (skew && (h <= skew_start))  /* OK to shift box for skewed text? */
            {
                rolw1(skew_mask);
                if (skew_mask & 0x8000)
                {
                    rorw1(src_mask);
                    if (src_mask == 0x8000)
                        src++;
                    dst += sizeof(UWORD);
                }
            }
        }
        break;
    case WM_XOR:
        for (h = vars->height; h > 0; h--, src += vars->s_next, dst += vars->d_next)
        {
            p = (UWORD *)src;
            q = (UWORD *)dst;
            for (w = vars->width, mask = src_mask; w > 0; w--)
            {
                if (*p & mask)
                    *q = ~*q;
                q++;
                rorw1(mask);
                if (mask == 0x8000)
                    p++;
            }
            /*
             * see comments for WM_REPLACE (above) for an explanation of
             * the following
             */
            if (skew && (h <= skew_start))  /* OK to shift box for skewed text? */
            {
                rolw1(skew_mask);
                if (skew_mask & 0x8000)
                {
                    rorw1(src_mask);
                    if (src_mask == 0x8000)
                        src++;
                    dst += sizeof(UWORD);
                }
            }
        }
        break;
    case WM_ERASE:
        for (h = vars->height; h > 0; h--, src += vars->s_next, dst += vars->d_next)
        {
            p = (UWORD *)src;
            q = (UWORD *)dst;
            for (w = vars->width, mask = src_mask; w > 0; w--)
            {
                /*
                 * behaviour here differs from TOS 4.04 - for further info,
                 * see the comments in direct_screen_blit16()
                 */
                if (!(*p & mask))
                    *q = fgcol;
                q++;
                rorw1(mask);
                if (mask == 0x8000)
                    p++;
            }
            /*
             * see comments for WM_REPLACE (above) for an explanation of
             * the following
             */
            if (skew && (h <= skew_start))  /* OK to shift box for skewed text? */
            {
                rolw1(skew_mask);
                if (skew_mask & 0x8000)
                {
                    rorw1(src_mask);
                    if (src_mask == 0x8000)
                        src++;
                    dst += sizeof(UWORD);
                }
            }
        }
        break;
    }
}


/*
 * ===========================================================================
 * VDI_RASTER_DRIVER entries
 * ===========================================================================
 */

static UWORD tc_get_pixel(WORD x, WORD y)
{
    return *get_start_addr16(x, y);     /* the pixel *is* the colour */
}


/*
 * NOTE: this does not work for Truecolor modes in TOS4 due to a bug.
 * Register a4 is used to reference the lineA pointer table, but has
 * never been set; the code should be using a1 instead.  So we can
 * safely assume that no existing program is expecting this to work.
 *
 * However, because EmuTOS aims to be better than TOS, a functioning
 * Truecolor mode has been implemented.  The EmuTOS Truecolor code
 * is based on what TOS4 apparently intends to do, i.e. just stores
 * the value passed as-is.  This also meshes with the operation of
 * linea2 in TOS4 Truecolor modes, which just retrieves the word at
 * the specified address.
 */
static void tc_put_pixel(WORD x, WORD y, UWORD color)
{
    UWORD *addr = get_start_addr16(x, y);

    /* co-ordinates can wrap, but we cannot write outside the screen */
    if (addr < (UWORD *)v_bas_ad || addr >= get_start_addr16(V_REZ_HZ, V_REZ_VT))
        return;

    *addr = color;
}


static WORD tc_find_span(const VwkClip *clip, WORD x, WORD y, UWORD search_color,
                         BOOL seed_type, WORD *xleftout, WORD *xrightout)
{
    /* see if we are in the y clipping range */
    if (y < clip->ymn_clip || y > clip->ymx_clip)
        return 0;

    return end_pts16(clip, x, y, search_color, seed_type, xleftout, xrightout);
}


/*
 * handle an undocumented feature of TOS4 VDI: you must be in a Truecolor
 * mode to get Truecolor device-dependent output from a 16-bit standard
 * form.  If the source has 8 planes or fewer, TOS4 transforms it to a
 * planar device-dependent form even in a Truecolor mode.
 */
static void tc_transform_form(MFDB *src, MFDB *dst)
{
    if (src->fd_nplanes > 8)
        vr_trnfm16(src, dst);
    else
        vdi_transform_form_planar(src, dst);
}


/*
 * screen_blit16() expects the values that used to be set up by the common
 * part of screen_blit(), so we do that here.
 */
static void tc_blit_glyph(LOCALVARS *vars)
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

    screen_blit16(vars);
}


/* in Truecolor modes, 0xffff is white */
static void tc_clear_screen(void)
{
    memset(v_bas_ad, 0xff, (ULONG)v_lin_wr * V_REZ_VT);
}


const VDI_RASTER_DRIVER vdi_raster_truecolor = {
    .name = "Falcon 16-bit",
    .resolution_changed = NULL,
    .get_pixel = tc_get_pixel,
    .put_pixel = tc_put_pixel,
    .fill_rect = swblit_rect_common16,
    .draw_line = draw_line16,
#if CONF_WITH_VDI_VERTLINE
    .draw_vertical_line = swblit_vertical_line16,
#endif
    .setup_forms = vdi_setup_forms_planar,
    .copy_raster_opaque = vdi_copy_raster_opaque_planar,
    .copy_raster_transparent = vdi_copy_raster_transparent_planar,
    .blit = blit_frame_copy,
    .transform_form = tc_transform_form,
    .find_span = tc_find_span,
    .blit_glyph = tc_blit_glyph,
#if CONF_WITH_VDI_TEXT_SPEEDUP
    .blit_string = direct_screen_blit16,
#endif
    .clear_screen = tc_clear_screen,
};

#endif /* CONF_WITH_VDI_16BIT */
