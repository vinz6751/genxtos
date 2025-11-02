/*
 * vdi_raster_line.c - Low level raster line drawing
 *
 * Copyright 1982 by Digital Research Inc.  All rights reserved.
 * Copyright 1999 by Caldera, Inc. and Authors:
 * Copyright (C) 2002-2025 The EmuTOS development team
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

 #include "emutos.h"
 #include "has.h"        /* for blitter-related items */
 #include "intmath.h"
 #include "asm.h"
 #include "aesext.h"
 #include "vdi_defs.h"
 #include "vdistub.h"
 #include "blitter.h"
 #include "tosvars.h"
 #include "biosext.h"    /* for cache control routines */
 #include "lineavars.h"
 #include "vdi_raster_line.h"
 #include "vdi_inline.h"

 /*
 * bit mask for 'standard' values of patmsk
 */
#define STD_PATMSKS ((1u<<15) | (1u<<7) | (1u<<3) | (1u<<1) | (1u<<0))

#if CONF_WITH_BLITTER
/*
 * blitter ops for draw/nodraw cases for wrt_mode 0-3
 */
const UBYTE op_draw[] = { 0x03, 0x07, 0x06, 0x0d };
const UBYTE op_nodraw[] = { 0x00, 0x04, 0x06, 0x01 };
#endif

/*
 * private structure for parameter passing
 */
 typedef struct
 {
     UWORD   leftmask;               /* left endmask */
     UWORD   rightmask;              /* right endmask */
     WORD    width;                  /* line width (in WORDs) */
     UWORD   *addr;                  /* starting screen address */
 } BLITPARM;


/*
 * set up values required by the horizontal line drawing functions
 *
 * This figures out the sizes of the left, centre, and right sections.
 * If the line lies entirely within a WORD, then the centre and right
 * section sizes will be zero; if the line spans two WORDs, then the
 * centre size will be zero.
 * It also initialises the screen pointer.
 */
 static __inline__ void draw_rect_setup(BLITPARM *b, const VwkAttrib *attr, const Rect *rect)
 {
     b->leftmask = 0xffff >> (rect->x1 & 0x0f);
     b->rightmask = 0xffff << (15 - (rect->x2 & 0x0f));
     b->width = (rect->x2 >> 4) - (rect->x1 >> 4) + 1;
     if (b->width == 1) {                /* i.e. all bits within 1 WORD */
         b->leftmask &= b->rightmask;    /* so combine masks */
         b->rightmask = 0;
     }
     b->addr = get_start_addr(rect->x1, rect->y1);   /* init address ptr */
 }
 
 
 #if CONF_WITH_BLITTER
 #if CONF_WITH_VDI_VERTLINE
 /*
  * draw a single vertical line using the blitter
  */
 void hwblit_vertical_line(const Line *line, WORD wrt_mode, UWORD color)
 {
     WORD i, plane, dy, yinc, height, start_line;
     UWORD mask;
     UWORD *screen_addr = get_start_addr(line->x1, line->y1);
     ULONG size;
 
     dy = line->y2 - line->y1;
     yinc = v_lin_wr;
 
     if (dy >= 0)
     {
         for (i = 0, mask = 0x8000; i < 16; i++, mask >>= 1)
             BLITTER->halftone[i] = (LN_MASK & mask) ? 0xffff : 0x0000;
         start_line = 0;
     }
     else
     {
         dy = -dy;
         yinc = -yinc;
         for (i = 0, mask = 0x0001; i < 16; i++, mask <<= 1)
             BLITTER->halftone[i] = (LN_MASK & mask) ? 0xffff : 0x0000;
         start_line = 15;
     }
 
     height = dy + 1;
     size = muls(height, v_lin_wr);
 
     /*
      * since the blitter doesn't see the data cache, and we may be in
      * copyback mode (e.g. the FireBee), we must flush the data cache
      * first to ensure that the screen memory is current.  the length
      * below should be correct, but note that the current cache control
      * routines ignore the length specification & act on the whole cache
      * anyway.
      */
     flush_data_cache(screen_addr, size);
 
     BLITTER->endmask_1 = 0x8000 >> (line->x1&0x000f);
     BLITTER->endmask_2 = 0x0000;
     BLITTER->endmask_3 = 0x0000;
     BLITTER->dst_y_incr = yinc;
     BLITTER->x_count = 1;
     BLITTER->hop = HOP_HALFTONE_ONLY;
     BLITTER->skew = 0;
 
     for (plane = 0; plane < v_planes; plane++, color >>= 1)
     {
         BLITTER->dst_addr = screen_addr++;
         BLITTER->y_count = dy + 1;
         BLITTER->op = (color & 1) ? op_draw[wrt_mode]: op_nodraw[wrt_mode];
 
         /*
          * we run the blitter in the Atari-recommended way: use no-HOG mode,
          * and manually restart the blitter until it's done.
          */
         BLITTER->status = BUSY | start_line;    /* no-HOG mode */
         __asm__ __volatile__(
         "lea    0xFFFF8A3C,a0\n\t"
         "0:\n\t"
         "tas    (a0)\n\t"
         "nop\n\t"
         "jbmi   0b\n\t"
         :
         :
         : "a0", "memory", "cc"
         );
     }
     /*
      * we've modified the screen behind the cpu's back, so we must
      * invalidate any cached screen data.
      */
     invalidate_data_cache(screen_addr, size);
 
     /* update LN_MASK for next time */
     mask = LN_MASK;
     for (i = height & 0x000f; i; i--)
         rolw1(mask);
     LN_MASK = mask;
 }
 #endif
 
 
 /*
  * hwblit_rect_nonstd: handle non-standard values of patmsk for hwblit_rect_common()
  *
  * we do a line-at-a-time within the normal plane-at-a-time loop
  *
  * NOTE: we rely on hwblit_rect_common() for the initial setup
  */
 void hwblit_rect_nonstd(const VwkAttrib *attr, const Rect *rect, UWORD *addr)
 {
     const UWORD patmsk = attr->patmsk;
     const UWORD *patptr = attr->patptr;
     UWORD color = attr->color;
     UWORD patindex = rect->y1 & patmsk;
     const WORD ycount = rect->y2 - rect->y1 + 1;
     UWORD *screen_addr = addr;
     int line, plane;
 
     for (plane = 0; plane < v_planes; plane++, color>>= 1)
     {
         BLITTER->dst_addr = screen_addr++;
         BLITTER->hop = HOP_HALFTONE_ONLY;
         BLITTER->op = (color & 1) ? op_draw[attr->wrt_mode]: op_nodraw[attr->wrt_mode];
 
         for (line = 0; line < ycount; line++)
         {
             BLITTER->halftone[0] = patptr[patindex++];
             if (patindex > patmsk)
                 patindex = 0;
             BLITTER->y_count = 1;
 
             /*
              * we run the blitter in the Atari-recommended way: use no-HOG mode,
              * and manually restart the blitter until it's done.
              */
             BLITTER->status = BUSY;
             __asm__ __volatile__(
             "lea    0xFFFF8A3C,a0\n\t"
             "0:\n\t"
             "tas    (a0)\n\t"
             "nop\n\t"
             "jbmi   0b\n\t"
             :
             :
             : "a0", "memory", "cc"
             );
         }
 
         if (attr->multifill)
             patptr += 16;
     }
 
     /*
      * invalidate any cached screen data
      */
     invalidate_data_cache(addr, v_lin_wr*ycount);
 }
 
 
 /*
  * hwblit_rect_common: blitter version of draw_rect_common
  *
  * Please refer to draw_rect_common for further information
  */
 void hwblit_rect_common(const VwkAttrib *attr, const Rect *rect)
 {
     const UWORD patmsk = attr->patmsk;
     const UWORD *patptr = attr->patptr;
     UWORD color = attr->color;
     const WORD ycount = rect->y2 - rect->y1 + 1;
     UWORD *screen_addr;
     UBYTE status;
     int i, plane;
     BLITPARM b;
     BOOL nonstd;
 
     /* set up masks, width, screen address pointer */
     draw_rect_setup(&b, attr, rect);
     screen_addr = b.addr;
 
     /*
      * flush the data cache to ensure that the screen memory is current
      */
     flush_data_cache(b.addr, v_lin_wr*ycount);
 
     BLITTER->src_x_incr = 0;
     BLITTER->endmask_1 = b.leftmask;
     BLITTER->endmask_2 = 0xffff;
     BLITTER->endmask_3 = b.rightmask;
     BLITTER->dst_x_incr = v_planes * sizeof(WORD);
     BLITTER->dst_y_incr = v_lin_wr - (v_planes*sizeof(WORD)*(b.width-1));
     BLITTER->x_count = b.width;
     BLITTER->skew = 0;
 
     /*
      * check for 'non-standard' values of patmsk:
      *  . if multifill is set, patmsk must be 15
      *  . if multifill is *not* set, patmsk must be 0, 1, 3, 7, or 15
      * if we have a non-standard value, we call a separate function
      */
     nonstd = FALSE;
     if (attr->multifill)
     {
         if (patmsk != 15)
             nonstd = TRUE;
     }
     else
     {
         if ((patmsk >= 16) || ((STD_PATMSKS & (1u<<patmsk)) == 0))
             nonstd = TRUE;
     }
     if (nonstd)
     {
         hwblit_rect_nonstd(attr, rect, b.addr);
         return;
     }
 
     status = BUSY | (rect->y1 & LINENO);    /* NOHOG mode */
 
     if (!attr->multifill)       /* only need to init halftone once */
     {
         for (i = 0; i < 16; i++)
             BLITTER->halftone[i] = patptr[i & patmsk];
     }
 
     for (plane = 0; plane < v_planes; plane++, color >>= 1)
     {
         if (attr->multifill)    /* need to init halftone each time */
         {
             UWORD *p = BLITTER->halftone;
             /* more efficient here because patmsk must be 15 */
             for (i = 0; i < 16; i++)
                 *p++ = *patptr++;
         }
         BLITTER->dst_addr = screen_addr++;
         BLITTER->y_count = ycount;
         BLITTER->hop = HOP_HALFTONE_ONLY;
         BLITTER->op = (color & 1) ? op_draw[attr->wrt_mode]: op_nodraw[attr->wrt_mode];
 
         /*
          * we run the blitter in the Atari-recommended way: use no-HOG mode,
          * and manually restart the blitter until it's done.
          */
         BLITTER->status = status;
         __asm__ __volatile__(
         "lea    0xFFFF8A3C,a0\n\t"
         "0:\n\t"
         "tas    (a0)\n\t"
         "nop\n\t"
         "jbmi   0b\n\t"
         :
         :
         : "a0", "memory", "cc"
         );
     }
 
     /*
      * invalidate any cached screen data
      */
     invalidate_data_cache(b.addr, v_lin_wr*ycount);
 }
 #endif
 

#if CONF_WITH_VDI_16BIT
/*
 * swblit_rect_common16 - draw one or more horizontal lines via software, 16-bit mode
 *
 * this is much simpler than the corresponding bitplane code (see swblit_rect_common())
 *
 * FIXME: notes to self
 * . do we need OPTIMIZE_SMALL?
 * . pay attention to notes in Compendium for vswr_mode re ERASE & TRANS
 */
void OPTIMIZE_SMALL swblit_rect_common16(const VwkAttrib *attr, const Rect *rect)
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
#endif


/*
 * swblit_rect_common - draw one or more horizontal lines via software
 *
 * This code does the following:
 *  1. The outermost control is via a switch() statement depending on
 *     the current drawing mode.
 *  2. Within each case, the outermost loop processes one scan line per
 *     iteration.
 *  3. Within this loop, the video planes are processed in sequence.
 *  4. Within this, the left section is processed, then the centre and/or
 *     right sections (if they exist).
 *
 * NOTE: this code seems rather longwinded and repetitive.  In fact it
 * can be shortened considerably and made much more elegant.  Doing so
 * however will wreck its performance, and this in turn will affect the
 * performance of many VDI calls.  This is not particularly noticeable
 * on an accelerated system, but is disastrous when running on a plain
 * 8MHz ST or 16MHz Falcon.  You are strongly advised not to change this
 * without a lot of careful thought & performance testing!
 */
void OPTIMIZE_SMALL swblit_rect_common(const VwkAttrib *attr, const Rect *rect)
 {
     const UWORD patmsk = attr->patmsk;
     const int vplanes = v_planes;
     const int yinc = (v_lin_wr>>1) - vplanes;
     int centre, y;
     BLITPARM b;
 
     /* set up masks, width, screen address pointer */
     draw_rect_setup(&b, attr, rect);
 
     centre = b.width - 2 - 1;   /* -1 because of the way we construct the innermost loops */
 
     switch(attr->wrt_mode) {
     case WM_ERASE:          /* erase (reverse transparent) mode */
         for (y = rect->y1; y <= rect->y2; y++, b.addr += yinc) {
             int patind = patmsk & y;   /* starting pattern */
             int plane;
             UWORD color;
 
             for (plane = 0, color = attr->color; plane < vplanes; plane++, color>>=1, b.addr++) {
                 UWORD *work = b.addr;
                 UWORD pattern = ~attr->patptr[patind];
                 int n;
 
                 if (color & 0x0001) {
                     *work |= pattern & b.leftmask;  /* left section */
                     work += vplanes;
 #ifdef __mcoldfire__
                     for (n = centre; n >= 0; n--) { /* centre section */
                         *work |= pattern;
                         work += vplanes;
                     }
 #else
                     if (centre >= 0) {              /* centre section */
                         n = centre;
                         __asm ("1:\n\t"
                                "or.w %2,(%1)\n\t"
                                "adda.w %3,%1\n\t"
                                "dbra %0,1b" : "+d"(n), "+a"(work) : "d"(pattern),
                                               "r"(2*vplanes) : "memory", "cc");
                     }
 #endif
                     if (b.rightmask) {              /* right section */
                         *work |= pattern & b.rightmask;
                     }
                 } else {
                     *work &= ~(pattern & b.leftmask);   /* left section */
                     work += vplanes;
 #ifdef __mcoldfire__
                     for (n = centre; n >= 0; n--) { /* centre section */
                         *work &= ~pattern;
                         work += vplanes;
                     }
 #else
                     if (centre >= 0) {              /* centre section */
                         n = centre;
                         __asm ("1:\n\t"
                                "and.w %2,(%1)\n\t"
                                "adda.w %3,%1\n\t"
                                "dbra %0,1b" : "+d"(n), "+a"(work) : "d"(~pattern),
                                               "r"(2*vplanes) : "memory", "cc");
                     }
 #endif
                     if (b.rightmask) {              /* right section */
                         *work &= ~(pattern & b.rightmask);
                     }
                 }
                 if (attr->multifill)
                     patind += 16;                   /* advance pattern data */
             }
         }
         break;
     case WM_XOR:            /* xor mode */
         for (y = rect->y1; y <= rect->y2; y++, b.addr += yinc) {
             int patind = patmsk & y;   /* starting pattern */
             int plane;
             UWORD color;
 
             for (plane = 0, color = attr->color; plane < vplanes; plane++, color>>=1, b.addr++) {
                 UWORD *work = b.addr;
                 UWORD pattern = attr->patptr[patind];
                 int n;
 
                 *work ^= pattern & b.leftmask;      /* left section */
                 work += vplanes;
 #ifdef __mcoldfire__
                 for (n = centre; n >= 0; n--) {    /* centre section */
                     *work ^= pattern;
                     work += vplanes;
                 }
 #else
                 if (centre >= 0) {                  /* centre section */
                     n = centre;
                     __asm ("1:\n\t"
                            "eor.w %2,(%1)\n\t"
                            "adda.w %3,%1\n\t"
                            "dbra %0,1b" : "+d"(n), "+a"(work) : "d"(pattern),
                                           "r"(2*vplanes) : "memory", "cc");
                 }
 #endif
                 if (b.rightmask) {                  /* right section */
                     *work ^= pattern & b.rightmask;
                 }
                 if (attr->multifill)
                     patind += 16;                   /* advance pattern data */
             }
         }
         break;
     case WM_TRANS:          /* transparent mode */
         for (y = rect->y1; y <= rect->y2; y++, b.addr += yinc) {
             int patind = patmsk & y;   /* starting pattern */
             int plane;
             UWORD color;
 
             for (plane = 0, color = attr->color; plane < vplanes; plane++, color>>=1, b.addr++) {
                 UWORD *work = b.addr;
                 UWORD pattern = attr->patptr[patind];
                 int n;
 
                 if (color & 0x0001) {
                     *work |= pattern & b.leftmask;  /* left section */
                     work += vplanes;
 #ifdef __mcoldfire__
                     for (n = centre; n >= 0; n--) { /* centre section */
                         *work |= pattern;
                         work += vplanes;
                     }
 #else
                     if (centre >= 0) {              /* centre section */
                         n = centre;
                         __asm ("1:\n\t"
                                "or.w %2,(%1)\n\t"
                                "adda.w %3,%1\n\t"
                                "dbra %0,1b" : "+d"(n), "+a"(work) : "d"(pattern),
                                               "r"(2*vplanes) : "memory", "cc");
                     }
 #endif
                     if (b.rightmask) {              /* right section */
                         *work |= pattern & b.rightmask;
                     }
                 } else {
                     *work &= ~(pattern & b.leftmask);   /* left section */
                     work += vplanes;
 #ifdef __mcoldfire__
                     for (n = centre; n >= 0; n--) { /* centre section */
                         *work &= ~pattern;
                         work += vplanes;
                     }
 #else
                     if (centre >= 0) {              /* centre section */
                         n = centre;
                         __asm ("1:\n\t"
                                "and.w %2,(%1)\n\t"
                                "adda.w %3,%1\n\t"
                                "dbra %0,1b" : "+d"(n), "+a"(work) : "d"(~pattern),
                                               "r"(2*vplanes) : "memory", "cc");
                     }
 #endif
                     if (b.rightmask) {              /* right section */
                         *work &= ~(pattern & b.rightmask);
                     }
                 }
                 if (attr->multifill)
                     patind += 16;                   /* advance pattern data */
             }
         }
         break;
     default:                /* replace mode */
         for (y = rect->y1; y <= rect->y2; y++, b.addr += yinc) {
             int patind = patmsk & y;   /* starting pattern */
             int plane;
             UWORD color;
 
             for (plane = 0, color = attr->color; plane < vplanes; plane++, color>>=1, b.addr++) {
                 UWORD data, *work = b.addr;
                 UWORD pattern = (color & 0x0001) ? attr->patptr[patind] : 0x0000;
                 int n;
 
                 data = *work & ~b.leftmask;         /* left section */
                 data |= pattern & b.leftmask;
                 *work = data;
                 work += vplanes;
 #ifdef __mcoldfire__
                 for (n = centre; n >= 0; n--) {     /* centre section */
                     *work = pattern;
                     work += vplanes;
                 }
 #else
                 if (centre >= 0) {                  /* centre section */
                     n = centre;
                     __asm ("1:\n\t"
                            "move.w %2,(%1)\n\t"
                            "adda.w %3,%1\n\t"
                            "dbra %0,1b" : "+d"(n), "+a"(work) : "r"(pattern),
                                           "r"(2*vplanes) : "memory", "cc");
                 }
 #endif
                 if (b.rightmask) {                  /* right section */
                     data = *work & ~b.rightmask;
                     data |= pattern & b.rightmask;
                     *work = data;
                 }
                 if (attr->multifill)
                     patind += 16;                   /* advance pattern data */
             }
         }
         break;
     }
 }
 

#if CONF_WITH_VDI_16BIT
/*
 * draw_line16 - draw a line (general purpose) in 16-bit graphics
 *
 * see draw_line() below for further info
 */
void draw_line16(const Line *line, WORD wrt_mode, UWORD color)
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
#endif


/*
 * draw_line - draw a line (general purpose)
 *
 * This routine draws a line defined by the Line structure, using
 * Bresenham's algorithm.  The line is modified by the LN_MASK
 * variable and the wrt_mode parameter.  This routine handles
 * all interleaved-bitplane video resolutions.
 *
 * Note that for line-drawing the background color is always 0
 * (i.e., there is no user-settable background color).  This fact
 * allows coding short-cuts in the implementation of "replace" and
 * "not" modes, resulting in faster execution of their inner loops.
 *
 * This routine is more or less the one from the original VDI asm
 * code, with the following exception:
 *  . When the writing mode was XOR, and this was not the last line
 *    in a polyline, the original code decremented the x coordinate
 *    of the ending point.  This prevented polylines from xor'ing
 *    themselves at the intermediate points.  The determination of
 *    'last line or not' was done via the LSTLIN variable which was
 *    set in the polyline() function.
 * We now handle this situation as follows:
 *  . The polyline() function still sets the LSTLIN variable.  The
 *    abline() function (q.v.) adjusts the line end coordinates
 *    accordingly.
 *
 */
 void draw_line(const Line *line, WORD wrt_mode, UWORD color)
 {
     UWORD *adr;
     WORD dx;                    /* width of rectangle around line */
     WORD dy;                    /* height of rectangle around line */
     WORD yinc;                  /* in/decrease for each y step */
     const WORD xinc = v_planes; /* positive increase for each x step, planes WORDS */
     UWORD msk;
     int plane;
     UWORD linemask = LN_MASK;   /* linestyle bits */
 
     dx = line->x2 - line->x1;
     dy = line->y2 - line->y1;
 
     /* calculate increase values for x and y to add to actual address */
     if (dy < 0) {
         dy = -dy;                       /* make dy absolute */
         yinc = (LONG) -1 * v_lin_wr / 2; /* sub one line of words */
     } else {
         yinc = (LONG) v_lin_wr / 2;     /* add one line of words */
     }
 
     adr = get_start_addr(line->x1, line->y1);   /* init address counter */
     msk = 0x8000 >> (line->x1&0xf);             /* initial bit position in WORD */
 
     for (plane = v_planes-1; plane >= 0; plane-- ) {
         UWORD *addr;
         WORD  eps;              /* epsilon */
         WORD  e1;               /* epsilon 1 */
         WORD  e2;               /* epsilon 2 */
         WORD  loopcnt;
         UWORD bit;
 
         /* load values fresh for this bitplane */
         addr = adr;             /* initial start address for changes */
         bit = msk;              /* initial bit position in WORD */
         linemask = LN_MASK;
 
         if (dx >= dy) {
             e1 = 2*dy;
             eps = -dx;
             e2 = 2*dx;
 
             switch (wrt_mode) {
             case WM_ERASE:      /* reverse transparent  */
                 if (color & 0x0001) {
                     for (loopcnt=dx;loopcnt >= 0;loopcnt--) {
                         rolw1(linemask);        /* get next bit of line style */
                         if (!(linemask&0x0001))
                             *addr |= bit;
                         rorw1(bit);
                         if (bit&0x8000)
                             addr += xinc;
                         eps += e1;
                         if (eps >= 0 ) {
                             eps -= e2;
                             addr += yinc;       /* increment y */
                         }
                     }
                 } else {
                     for (loopcnt=dx;loopcnt >= 0;loopcnt--) {
                         rolw1(linemask);        /* get next bit of line style */
                         if (!(linemask&0x0001))
                             *addr &= ~bit;
                         rorw1(bit);
                         if (bit&0x8000)
                             addr += xinc;
                         eps += e1;
                         if (eps >= 0 ) {
                             eps -= e2;
                             addr += yinc;       /* increment y */
                         }
                     }
                 }
                 break;
             case WM_XOR:        /* xor */
                 for (loopcnt=dx;loopcnt >= 0;loopcnt--) {
                     rolw1(linemask);        /* get next bit of line style */
                     if (linemask&0x0001)
                         *addr ^= bit;
                     rorw1(bit);
                     if (bit&0x8000)
                         addr += xinc;
                     eps += e1;
                     if (eps >= 0 ) {
                         eps -= e2;
                         addr += yinc;       /* increment y */
                     }
                 }
                 break;
             case WM_TRANS:      /* or */
                 if (color & 0x0001) {
                     for (loopcnt=dx;loopcnt >= 0;loopcnt--) {
                         rolw1(linemask);        /* get next bit of line style */
                         if (linemask&0x0001)
                             *addr |= bit;
                         rorw1(bit);
                         if (bit&0x8000)
                             addr += xinc;
                         eps += e1;
                         if (eps >= 0 ) {
                             eps -= e2;
                             addr += yinc;       /* increment y */
                         }
                     }
                 } else {
                     for (loopcnt=dx;loopcnt >= 0;loopcnt--) {
                         rolw1(linemask);        /* get next bit of line style */
                         if (linemask&0x0001)
                             *addr &= ~bit;
                         rorw1(bit);
                         if (bit&0x8000)
                             addr += xinc;
                         eps += e1;
                         if (eps >= 0 ) {
                             eps -= e2;
                             addr += yinc;       /* increment y */
                         }
                     }
                 }
                 break;
             case WM_REPLACE:    /* rep */
                 if (color & 0x0001) {
                     for (loopcnt=dx;loopcnt >= 0;loopcnt--) {
                         rolw1(linemask);        /* get next bit of line style */
                         if (linemask&0x0001)
                             *addr |= bit;
                         else
                             *addr &= ~bit;
                         rorw1(bit);
                         if (bit&0x8000)
                             addr += xinc;
                         eps += e1;
                         if (eps >= 0 ) {
                             eps -= e2;
                             addr += yinc;       /* increment y */
                         }
                     }
                 }
                 else {
                     for (loopcnt=dx;loopcnt >= 0;loopcnt--) {
                         rolw1(linemask);        /* get next bit of line style */
                         *addr &= ~bit;
                         rorw1(bit);
                         if (bit&0x8000)
                             addr += xinc;
                         eps += e1;
                         if (eps >= 0 ) {
                             eps -= e2;
                             addr += yinc;       /* increment y */
                         }
                     }
                 }
             }
         } else {
             e1 = 2*dx;
             eps = -dy;
             e2 = 2*dy;
 
             switch (wrt_mode) {
             case WM_ERASE:      /* reverse transparent */
                 if (color & 0x0001) {
                     for (loopcnt=dy;loopcnt >= 0;loopcnt--) {
                         rolw1(linemask);        /* get next bit of line style */
                         if (!(linemask&0x0001))
                             *addr |= bit;
                         addr += yinc;
                         eps += e1;
                         if (eps >= 0 ) {
                             eps -= e2;
                             rorw1(bit);
                             if (bit&0x8000)
                                 addr += xinc;
                         }
                     }
                 } else {
                     for (loopcnt=dy;loopcnt >= 0;loopcnt--) {
                         rolw1(linemask);        /* get next bit of line style */
                         if (!(linemask&0x0001))
                             *addr &= ~bit;
                         addr += yinc;
                         eps += e1;
                         if (eps >= 0 ) {
                             eps -= e2;
                             rorw1(bit);
                             if (bit&0x8000)
                                 addr += xinc;
                         }
                     }
                 }
                 break;
             case WM_XOR:        /* xor */
                 for (loopcnt=dy;loopcnt >= 0;loopcnt--) {
                     rolw1(linemask);        /* get next bit of line style */
                     if (linemask&0x0001)
                         *addr ^= bit;
                     addr += yinc;
                     eps += e1;
                     if (eps >= 0 ) {
                         eps -= e2;
                         rorw1(bit);
                         if (bit&0x8000)
                             addr += xinc;
                     }
                 }
                 break;
             case WM_TRANS:      /* or */
                 if (color & 0x0001) {
                     for (loopcnt=dy;loopcnt >= 0;loopcnt--) {
                         rolw1(linemask);        /* get next bit of line style */
                         if (linemask&0x0001)
                             *addr |= bit;
                         addr += yinc;
                         eps += e1;
                         if (eps >= 0 ) {
                             eps -= e2;
                             rorw1(bit);
                             if (bit&0x8000)
                                 addr += xinc;
                         }
                     }
                 } else {
                     for (loopcnt=dy;loopcnt >= 0;loopcnt--) {
                         rolw1(linemask);        /* get next bit of line style */
                         if (linemask&0x0001)
                             *addr &= ~bit;
                         addr += yinc;
                         eps += e1;
                         if (eps >= 0 ) {
                             eps -= e2;
                             rorw1(bit);
                             if (bit&0x8000)
                                 addr += xinc;
                         }
                     }
                 }
                 break;
             case WM_REPLACE:    /* rep */
                 if (color & 0x0001) {
                     for (loopcnt=dy;loopcnt >= 0;loopcnt--) {
                         rolw1(linemask);        /* get next bit of line style */
                         if (linemask&0x0001)
                             *addr |= bit;
                         else
                             *addr &= ~bit;
                         addr += yinc;
                         eps += e1;
                         if (eps >= 0 ) {
                             eps -= e2;
                             rorw1(bit);
                             if (bit&0x8000)
                                 addr += xinc;
                         }
                     }
                 }
                 else {
                     for (loopcnt=dy;loopcnt >= 0;loopcnt--) {
                         rolw1(linemask);        /* get next bit of line style */
                         *addr &= ~bit;
                         addr += yinc;
                         eps += e1;
                         if (eps >= 0 ) {
                             eps -= e2;
                             rorw1(bit);
                             if (bit&0x8000)
                                 addr += xinc;
                         }
                     }
                 }
             }
         }
         adr++;
         color >>= 1;    /* shift color index: next plane */
     }
     LN_MASK = linemask;
 }
 

#if CONF_WITH_VDI_VERTLINE
#if CONF_WITH_VDI_16BIT
/*
 * swblit_vertical_line16 - draw a vertical line in 16-bit graphics
 */
void swblit_vertical_line16(const Line *line, WORD wrt_mode, UWORD color)
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
 * vertical_line - draw a vertical line
 *
 * this is a stripped-down version of abline(), for speed
 */
void vertical_line(const Line *line, WORD wrt_mode, UWORD color)
{
    UWORD *start, *addr;
    WORD dy;                    /* length of line */
    WORD yinc;                  /* in/decrease for each y step */
    UWORD bit, bitcomp;
    WORD plane, loopcnt;
    UWORD linemask = LN_MASK;

    /* calculate increase value for y to add to actual address */
    dy = line->y2 - line->y1;
    yinc = v_lin_wr / 2;        /* one line of words */

    if (dy < 0) {
        dy = -dy;               /* make dy absolute */
        yinc = -yinc;           /* sub one line of words */
    }

    start = get_start_addr(line->x1, line->y1); /* init address counter */
    bit = 0x8000 >> (line->x1&0xf);             /* initial bit position in WORD */
    bitcomp = ~bit;

    for (plane = v_planes-1; plane >= 0; plane--, start++, color >>= 1) {
        /* load values fresh for this bitplane */
        addr = start;           /* initial start address for changes */
        linemask = LN_MASK;

        switch(wrt_mode) {
        case WM_ERASE:          /* reverse transparent */
            if (color & 0x0001) {
                for (loopcnt = dy; loopcnt >= 0; loopcnt--) {
                    rolw1(linemask);        /* get next bit of line style */
                    if (!(linemask&0x0001))
                        *addr |= bit;
                    addr += yinc;
                }
            } else {
                for (loopcnt = dy; loopcnt >= 0; loopcnt--) {
                    rolw1(linemask);        /* get next bit of line style */
                    if (!(linemask&0x0001))
                        *addr &= bitcomp;
                    addr += yinc;
                }
            }
            break;
        case WM_XOR:            /* xor */
            for (loopcnt = dy; loopcnt >= 0; loopcnt--) {
                rolw1(linemask);        /* get next bit of line style */
                if (linemask&0x0001)
                    *addr ^= bit;
                addr += yinc;
            }
            break;
        case WM_TRANS:          /* or */
            if (color & 0x0001) {
                for (loopcnt = dy; loopcnt >= 0; loopcnt--) {
                    rolw1(linemask);        /* get next bit of line style */
                    if (linemask&0x0001)
                        *addr |= bit;
                    addr += yinc;
                }
            } else {
                for (loopcnt = dy; loopcnt >= 0; loopcnt--) {
                    rolw1(linemask);        /* get next bit of line style */
                    if (linemask&0x0001)
                        *addr &= bitcomp;
                    addr += yinc;
                }
            }
            break;
        case WM_REPLACE:        /* rep */
            if (color & 0x0001) {
                for (loopcnt = dy; loopcnt >= 0; loopcnt--) {
                    rolw1(linemask);        /* get next bit of line style */
                    if (linemask&0x0001)
                        *addr |= bit;
                    else
                        *addr &= bitcomp;
                    addr += yinc;
                }
            } else {
                for (loopcnt = dy;loopcnt >= 0; loopcnt--) {
                    rolw1(linemask);        /* get next bit of line style */
                    *addr &= bitcomp;
                    addr += yinc;
                }
            }
        }
    }
    LN_MASK = linemask;
}
#endif
