/*
 * vdi_line.c - Line drawing
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


#define ABS(x) ((x) >= 0 ? (x) : -(x))


#define MAX_PIXEL_ASPECT_RATIO  2   /* max expected value of xsize/ysize */
#define MAX_QC_LINES    ((MAX_LINE_WIDTH*MAX_PIXEL_ASPECT_RATIO)/2 + 1)


/* the six predefined line styles */
const UWORD LINE_STYLE[6] = { 0xFFFF, 0xFFF0, 0xC0C0, 0xFF18, 0xFF00, 0xF191 };

/*
 * The following array holds values that allow wideline() to draw a
 * rasterized circle.  The values are actually those required for a
 * quarter of the circle, specifically quadrant 1.  [Quadrants are
 * numbered 1-4, beginning with the "south-east" quadrant, and
 * travelling clockwise].
 *
 * q_circle[n] contains the offset of the edge of the circle (from a
 * vertical line through the centre of the circle), for the nth line
 * (counting from a horizontal line through the centre of the circle).
 */
static WORD q_circle[MAX_QC_LINES]; /* Holds the circle DDA */

/* Wide line attribute save areas */
static WORD s_begsty, s_endsty, s_fil_col, s_fill_per;


/*
 * vdi_vsl_udsty - set user-defined line style
 */
void vdi_vsl_udsty(Vwk * vwk)
{
    vwk->ud_ls = INTIN[0];
}


/*
 * vdi_vsl_type - Set line type for line-drawing functions
 */
void vdi_vsl_type(Vwk * vwk)
{
    WORD li;

    li = ((INTIN[0]<MIN_LINE_STYLE) || (INTIN[0]>MAX_LINE_STYLE)) ? DEF_LINE_STYLE : INTIN[0];

    INTOUT[0] = li;
    vwk->line_index = li - 1;
}


/*
 * vdi_vsl_width - Set line width
 */
void vdi_vsl_width(Vwk * vwk)
{
    WORD w;

    /* Limit the requested line width to a reasonable value. */
    w = PTSIN[0];
    if (w < 1)
        w = 1;
    else if (w > SIZ_TAB[6])
        w = SIZ_TAB[6];

    /* If the line width is even, make it odd by decreasing it by one */
    if ((w & 0x0001) == 0)
        w--;

    /* Set the line width internals and return parameters */
    PTSOUT[0] = vwk->line_width = w;
    PTSOUT[1] = 0;
}


/*
 * vdi_vsl_ends - sets the style of end point for line starting and ending points
 */
void vdi_vsl_ends(Vwk * vwk)
{
    WORD lb, le;

    lb = ((INTIN[0] < MIN_END_STYLE) || (INTIN[0] > MAX_END_STYLE)) ? DEF_END_STYLE : INTIN[0];
    le = ((INTIN[1] < MIN_END_STYLE) || (INTIN[1] > MAX_END_STYLE)) ? DEF_END_STYLE : INTIN[1];

    INTOUT[0] = vwk->line_beg = lb;
    INTOUT[1] = vwk->line_end = le;
}


/*
 * vdi_vsl_color - sets the color for line-drawing
 */
void vdi_vsl_color(Vwk * vwk)
{
    WORD lc;

    lc = linea_validate_color_index(INTIN[0]);
    INTOUT[0] = lc;
    vwk->line_color = MAP_COL[lc];
}


/*
 * vdi_vql_attributes - Inquire current polyline attributes
 */
void vdi_vql_attributes(Vwk * vwk)
{
    INTOUT[0] = vwk->line_index + 1;
    INTOUT[1] = REV_MAP_COL[vwk->line_color];
    INTOUT[2] = vwk->wrt_mode + 1;

    PTSOUT[0] = vwk->line_width;
    PTSOUT[1] = 0;
}



/*
 * draw_rect_common - draw one or more horizontal lines
 *
 * calls the hardware or software blitter code to perform the blit
 */
void draw_rect_common(const VwkAttrib *attr, const Rect *rect)
{
#if CONF_WITH_VDI_16BIT
    if (TRUECOLOR_MODE)
        swblit_rect_common16(attr, rect);
    else
#endif
#if CONF_WITH_BLITTER
    if (blitter_is_enabled)
    {
        hwblit_rect_common(attr, rect);
    }
    else
#endif
    {
        swblit_rect_common(attr, rect);
    }
}


/*
 * helper to copy relevant Vwk members to the VwkAttrib struct, which is
 * used to pass the required Vwk info from VDI/Line-A polygon drawing to
 * draw_rect().
 */
void Vwk2Attrib(const Vwk *vwk, VwkAttrib *attr, const UWORD color)
{
    /* in the same order as in Vwk, so that GCC
     * can use longs for copying words
     */
    attr->clip = vwk->clip;
    attr->multifill = vwk->multifill;
    attr->patmsk = vwk->patmsk;
    attr->patptr = vwk->patptr;
    attr->wrt_mode = vwk->wrt_mode;
    attr->color = color;
}


/*
 * VDI wrapper for draw_rect_common
 */
void draw_rect(const Vwk * vwk, Rect * rect, const UWORD fillcolor)
{
    VwkAttrib attr;

    Vwk2Attrib(vwk, &attr, fillcolor);
    draw_rect_common(&attr, rect);
}


/*
 * VDI helper/wrapper for horizontal line drawing
 */
static __inline__ void horzline(const Vwk * vwk, Line * line)
{
    /* a horizontal line is a rectangle with one pixel height */
    arb_corner((Rect *)line);
    draw_rect(vwk, (Rect*)line, vwk->line_color);
}


/*
 * compose color for draw_rect_common & abline from line-A variables
 */
static UWORD linea_color(void)
{
    UWORD color = 0;

    /* Below we use += instead of |= because GCC produces better code
     * especially addq.w instead of ori.w
     */

    if (COLBIT0 != 0)
        color += 1;

    if (COLBIT1 != 0)
        color += 2;

    if (COLBIT2 != 0)
        color += 4;

    if (COLBIT3 != 0)
        color += 8;

    return color;
}


/*
 * lineA2Attrib - sets VwkAttrib fields from line-A variables
 */
static void lineA2Attrib(VwkAttrib *attr)
{
    attr->clip = CLIP;      /* only used by polygon drawing */
    if (PATPTR) {
        attr->patmsk = PATMSK;
        attr->patptr = PATPTR;
    } else {
        /* pattern is always needed for draw_rect_common, default to solid */
        attr->patmsk = 0;
        attr->patptr = &SOLID;
    }
    attr->wrt_mode = WRT_MODE;
    attr->color = linea_color();
}


/*
 * Line-A wrapper for draw_rect_common
 */
void linea_rect(void)
{
    VwkAttrib attr;
    Rect line;

    if (CLIP) {
        if (X1 < XMINCL)
            X1 = XMINCL;
        if (X2 > XMAXCL)
            X2 = XMAXCL;
        if (Y1 < YMINCL)
            Y1 = YMINCL;
        if (Y2 > YMAXCL)
            Y2 = YMAXCL;
    }
    line.x1 = X1;
    line.x2 = X2;
    line.y1 = Y1;
    line.y2 = Y2;

    lineA2Attrib(&attr);
    attr.multifill = MFILL;         /* linea5 supports MFILL */
    draw_rect_common(&attr, &line);
}


/*
 * Line-A wrapper for horizontal line
 */
void linea_hline(void)
{
    VwkAttrib attr;
    Rect line;

    line.x1 = X1;
    line.x2 = X2;
    line.y1 = Y1;
    line.y2 = Y1;

    lineA2Attrib(&attr);
    attr.multifill = MFILL;         /* linea4 supports MFILL */
    draw_rect_common(&attr, &line);
}


/*
 * Line-A wrapper for clc_flit
 */
void linea_polygon(void)
{
    VwkClip clipper;
    Point *points = (Point*) PTSIN;
    int count = CONTRL[1];
    VwkAttrib attr;

    lineA2Attrib(&attr);
    attr.multifill = 0;         /* linea6 does not support MFILL */
    if (CLIP) {
        /* clc_flit does only X-clipping */
        clipper.xmn_clip = XMINCL;
        clipper.xmx_clip = XMAXCL;
    } else {
        clipper.xmn_clip = 0;
        clipper.xmx_clip = xres;
    }
    clc_flit(&attr, &clipper, points, count, Y1, Y1-1);
}


/*
 * Line-A wrapper for floodfill
 */
void linea_fill(void)
{
    VwkClip clipper;
    VwkAttrib attr;
    lineA2Attrib(&attr);
    attr.multifill = 0;         /* lineaf does not support MFILL */
    attr.color = CUR_WORK->fill_color;
    if (CLIP) {
        clipper.xmn_clip = XMINCL;
        clipper.xmx_clip = XMAXCL;
        clipper.ymn_clip = YMINCL;
        clipper.ymx_clip = YMAXCL;
    } else {
        clipper.xmn_clip = 0;
        clipper.xmx_clip = xres;
        clipper.ymn_clip = 0;
        clipper.ymx_clip = yres;
    }
    contourfill(&attr, &clipper);
}


/*
 * vdi_v_pline - wrapper for polyline/wideline
 */
void vdi_v_pline(Vwk * vwk)
{
    Point * point = (Point*)PTSIN;
    int count = CONTRL[1];

    set_LN_MASK(vwk);

#if HAVE_BEZIER
    /* check, if we want to draw a bezier curve */
    if (CONTRL[5] == 13 && vwk->bez_qual )        /* FIXME: bez_qual ok?? */
        v_bez(vwk, point, count);
    else
#endif
    {
        if (vwk->line_width == 1) {
            polyline(vwk, point, count, vwk->line_color);
            if ((vwk->line_beg | vwk->line_end) & ARROWED)
                arrow(vwk, point, count);
        } else
            wideline(vwk, point, count);
    }
}


/*
 * clip_code - helper function, used by clip_line()
 *
 * returns a bit mask indicating where x and y are, relative
 * to the clipping rectangle:
 *  1   x is left
 *  2   x is right
 *  4   y is above
 *  8   y is below
 */
static WORD clip_code(Vwk * vwk, WORD x, WORD y)
{
    WORD clip_flag;

    clip_flag = 0;
    if (x < vwk->xmn_clip)
        clip_flag = 1;
    else if (x > vwk->xmx_clip)
        clip_flag = 2;
    if (y < vwk->ymn_clip)
        clip_flag += 4;
    else if (y > vwk->ymx_clip)
        clip_flag += 8;
    return (clip_flag);
}


/*
 * clip_line - clip line if necessary
 *
 * returns FALSE iff the line lies outside the clipping rectangle
 * otherwise, updates the contents of the Line structure & returns TRUE
 */
BOOL clip_line(Vwk * vwk, Line * line)
{
    WORD deltax, deltay, x1y1_clip_flag, x2y2_clip_flag, line_clip_flag;
    WORD *x, *y;

    while ((x1y1_clip_flag = clip_code(vwk, line->x1, line->y1)) |
           (x2y2_clip_flag = clip_code(vwk, line->x2, line->y2))) {
        if ((x1y1_clip_flag & x2y2_clip_flag))
            return (FALSE);
        if (x1y1_clip_flag) {
            line_clip_flag = x1y1_clip_flag;
            x = &line->x1;
            y = &line->y1;
        } else {
            line_clip_flag = x2y2_clip_flag;
            x = &line->x2;
            y = &line->y2;
        }
        deltax = line->x2 - line->x1;
        deltay = line->y2 - line->y1;
        if (line_clip_flag & 1) {               /* left ? */
            *y = line->y1 + mul_div_round(deltay, (vwk->xmn_clip-line->x1), deltax);
            *x = vwk->xmn_clip;
        } else if (line_clip_flag & 2) {        /* right ? */
            *y = line->y1 + mul_div_round(deltay, (vwk->xmx_clip-line->x1), deltax);
            *x = vwk->xmx_clip;
        } else if (line_clip_flag & 4) {        /* top ? */
            *x = line->x1 + mul_div_round(deltax, (vwk->ymn_clip-line->y1), deltay);
            *y = vwk->ymn_clip;
        } else if (line_clip_flag & 8) {        /* bottom ? */
            *x = line->x1 + mul_div_round(deltax, (vwk->ymx_clip-line->y1), deltay);
            *y = vwk->ymx_clip;
        }
    }
    return (TRUE);              /* segment now clipped  */
}


/*
 * polyline - draw a poly-line
 *
 * note: we pass the colour, since this routine is also used for
 * perimeters, which are drawn in the fill colour ...
 */
void polyline(Vwk * vwk, Point * point, int count, WORD color)
{
    int i;
    Line line;

    for (i = count-1, LSTLIN = FALSE; i > 0; i--) {
        if (i == 1)
            LSTLIN = TRUE;
        line.x1 = point->x;
        line.y1 = point->y;
        point++;                /* advance point by point */
        line.x2 = point->x;
        line.y2 = point->y;

        if (vwk->clip)
            if (!clip_line(vwk, &line))
                continue;

        abline(&line, vwk->wrt_mode, color);
    }
}


/*
 * quad_xform - helper function for perp_off()
 *
 * Converts input (x,y) to output (x,y) according to the value in 'quad':
 *  1 ("south-east" quadrant):  x -> x,  y -> y
 *  2 ("south-west" quadrant):  x -> -x, y -> y
 *  3 ("north-west" quadrant):  x -> -x, y -> -y
 *  4 ("north-east" quadrant):  x -> x,  y -> -y
 */
static void quad_xform(WORD quad, WORD x, WORD y, WORD *tx, WORD *ty)
{
    switch (quad) {
    case 1:
    case 4:
        *tx = x;
        break;

    case 2:
    case 3:
        *tx = -x;
        break;
    }

    switch (quad) {
    case 1:
    case 2:
        *ty = y;
        break;

    case 3:
    case 4:
        *ty = -y;
        break;
    }
}


/*
 * perp_off - calculate the perpendicular offsets
 *
 * Given a vector (vx,vy) which specifies the length and direction of
 * a line segment, this function returns x & y offsets to add/subtract
 * to the endpoints of the line segment.  The four points thereby
 * specified form a box which is the wideline segment.
 */
static void perp_off(WORD * px, WORD * py)
{
    WORD *vx, *vy, *pcircle, u, v;
    WORD x, y, quad, magnitude, min_val;
    WORD x_val = 0;
    WORD y_val = 0;

    vx = px;
    vy = py;

    pcircle = q_circle;

    /* Mirror transform the vector so that it is in the first quadrant. */
    if (*vx >= 0)
        quad = (*vy >= 0) ? 1 : 4;
    else
        quad = (*vy >= 0) ? 2 : 3;

    quad_xform(quad, *vx, *vy, &x, &y);

    /*
     * Traverse the circle in a dda-like manner and find the coordinate
     * pair (u, v) such that the magnitude of (u*y - v*x) is minimized.
     * In case of a tie, choose the value which causes (u - v) to be
     * minimized.  If not possible, do something.
     */
    min_val = 32767;
    u = *pcircle;
    v = 0;
    while (TRUE) {
        /* Check for new minimum, same minimum, or finished. */
        magnitude = ABS(u * y - v * x);
        if ((magnitude < min_val) ||
            ((magnitude == min_val) && (ABS(x_val - y_val) > ABS(u - v)))) {
            min_val = magnitude;
            x_val = u;
            y_val = v;
        }
        else
            break;

        /* Step to the next pixel. */
        if (v == num_qc_lines - 1) {
            if (u == 1)
                break;
            else
                u--;
        }
        else {
            if (pcircle[v + 1] >= u - 1) {
                v++;
                u = pcircle[v];
            } /* End if:  do next row up. */
            else {
                u--;
            }                   /* End else:  continue on row. */
        }                       /* End else:  other than top row. */
    }                           /* End FOREVER loop. */

    /* Transform the solution according to the quadrant. */
    quad_xform(quad, x_val, y_val, vx, vy);
}


/*
 * cir_dda - populate q_circle[] array
 *
 * This is called by wideline() when the current wideline width (line_cw)
 * changes, in order to reinitialise q_circle[].  It uses Bresenham's
 * circle algorithm.
 */
static void cir_dda(WORD line_width)
{
    WORD i, j, m, n;
    WORD *xptr, *yptr, x, y, d;

    /* Calculate the number of vertical pixels required. */
    num_qc_lines = (line_width * xsize / ysize) / 2 + 1;
    if (num_qc_lines > MAX_QC_LINES)
        num_qc_lines = MAX_QC_LINES;    /* circles will be flattened */

    /* Initialize the circle DDA.  "y" is set to the radius. */
    x = 0;
    y = (line_width + 1) / 2;
    d = 3 - 2 * y;

    /* Do an octant, starting at north.  The values for the next octant */
    /* (clockwise) will be filled by transposing x and y.               */
    while (x <= y) {
        q_circle[y] = x;
        q_circle[x] = y;

        if (d < 0)
            d += 4 * x + 6;
        else {
            d += 4 * (x - y) + 10;
            y--;
        }
        x++;
    }

    if (xsize == ysize)     /* square pixels, e.g. ST high */
        return;

    /*
     * handle tall pixels, e.g. ST medium, Falcon 640x240
     *
     * note that this can also include ST Low, which has "slightly tall"
     * pixels on pre-TOS3 machines
     */
    if (xsize < ysize) {
        /* Fake pixel averaging */
        x = 0;

        yptr = q_circle;
        for (i = 0; i < num_qc_lines; i++) {
            y = ((2 * i + 1) * ysize / xsize) / 2;
            d = 0;

            xptr = &q_circle[x];
            for (j = x; j <= y; j++)
                d += *xptr++;

            *yptr++ = d / (y - x + 1);
            x = y + 1;
        }
        return;
    }

    /*
     * handle xsize > ysize (wide pixels, e.g. Falcon 320x480)
     *
     * we interpolate from the previously-generated table.  for now we
     * greatly simplify things by assuming that xsize = 2 * ysize (true
     * for the Falcon).  the following code sets:
     *  q_circle[i] = q_circle[i/2]                         (for even i)
     *  q_circle[i] = (q_circle[i/2] + q_circle[i/2+1])/2   (for odd i)
     */
    for (i = num_qc_lines-1, n = 0; i > 0; i--) {
        m = q_circle[i/2];
        q_circle[i] = (m + n) / 2;
        n = m;
    }
}


/*
 * do_circ - draw a circle
 *
 * This is used by wideline():
 *  a) to round the ends of the line if not SQUARED
 *  b) to make a smooth join between line segments of a polyline
 */
static void do_circ(Vwk * vwk, WORD cx, WORD cy)
{
    Line line;
    WORD k;
    WORD *pointer;

    /* Do the upper and lower semi-circles. */
    for (k = 0, pointer = q_circle; k < num_qc_lines; k++, pointer++) {
        /* Upper semi-circle, plus the horizontal line through the center of the circle. */
        line.x1 = cx - *pointer;
        line.x2 = cx + *pointer;
        line.y1 = cy - k;
        line.y2 = cy - k;
        if (clip_line(vwk, &line))
            horzline(vwk, &line);

        if (k == 0)
            continue;

        /* Lower semi-circle. */
        line.x1 = cx - *pointer;
        line.x2 = cx + *pointer;
        line.y1 = cy + k;
        line.y2 = cy + k;
        if (clip_line(vwk, &line))
            horzline(vwk, &line);
    }
}


/*
 * s_fa_attr - Save the fill area attribute
 */
static void s_fa_attr(Vwk * vwk)
{
    /* Set up the fill area attribute environment. */
    LN_MASK = LINE_STYLE[0];
    s_fil_col = vwk->fill_color;
    s_fill_per = vwk->fill_per;
    s_begsty = vwk->line_beg;
    s_endsty = vwk->line_end;

    vwk->fill_color = vwk->line_color;
    vwk->line_beg = SQUARED;
    vwk->line_end = SQUARED;
    vwk->fill_per = TRUE;
    vwk->patptr = (UWORD *)&SOLID;
    vwk->patmsk = 0;
}                               /* End "s_fa_attr". */


/*
 * r_fa_attr - Restore the fill area attribute
 */
static void r_fa_attr(Vwk * vwk)
{
    /* Restore the fill area attribute environment. */
    vwk->fill_color = s_fil_col;
    vwk->fill_per = s_fill_per;
    vwk->line_beg = s_begsty;
    vwk->line_end = s_endsty;
}                               /* End "r_fa_attr". */


/*
 * wideline - draw a line with width >1
 */
void wideline(Vwk * vwk, Point * point, int count)
{
    WORD i, k;
    WORD wx1, wy1, wx2, wy2, vx, vy;
    BOOL closed = FALSE;
    Point *ptr, box[5];      /* box must be large enough to close polygon */

    /* Don't attempt wide lining on a degenerate polyline */
    if (count < 2)
        return;

    /* See if we need to rebuild q_circle[] */
    if (vwk->line_width != line_cw) {
        line_cw = vwk->line_width;
        cir_dda(line_cw);
    }

    /* If the ends are arrowed, output them. */
    if ((vwk->line_beg | vwk->line_end) & ARROWED)
        arrow(vwk, point, count);

    s_fa_attr(vwk);

    /* Initialize the starting point for the loop. */
    wx1 = point->x;
    wy1 = point->y;

    /* Determine if the line is a closed polyline */
    ptr = point + count - 1;    /* point to last vertex */
    if ((ptr->x == wx1) && (ptr->y == wy1))
        closed = TRUE;

    /*
     * If the end style for the first point is not squared,
     * or the polyline is closed, output a circle
     */
    if ((s_begsty != SQUARED) || closed)
        do_circ(vwk, wx1, wy1);

    /* Loop over the number of points passed in. */
    for (i = 1; i < count; i++) {
        /* Get ending point for line segment */
        point++;
        wx2 = point->x;
        wy2 = point->y;

        /* Get vector from start to end of the segment. */
        vx = wx2 - wx1;
        vy = wy2 - wy1;

        /* Ignore lines of zero length. */
        if ((vx == 0) && (vy == 0))
            continue;

        /* Calculate offsets to fatten the line. */
        if (vx == 0) {      /* line is vertical - do it the simple way */
            vx = q_circle[0];
            vy = 0;
        }
        else if (vy == 0) { /* line is horizontal - even simpler */
            vx = 0;
            vy = num_qc_lines - 1;
        }
        else {              /* neither */
            /* Find the offsets in x and y for a point perpendicular */
            /* to the line segment at the appropriate distance. */
            k = mul_div(-vy, ysize, xsize);
            vy = mul_div(vx, xsize, ysize);
            vx = k;
            perp_off(&vx, &vy);
        }

        /* Prepare the control and points parameters for the polygon call. */
        ptr = box;
        ptr->x = wx1 + vx;
        ptr->y = wy1 + vy;

        ptr++;
        ptr->x = wx1 - vx;
        ptr->y = wy1 - vy;

        ptr++;
        ptr->x = wx2 - vx;
        ptr->y = wy2 - vy;

        ptr++;
        ptr->x = wx2 + vx;
        ptr->y = wy2 + vy;

        polygon(vwk, box, 4);

        /*
         * If the terminal point of the line segment is an internal joint,
         * or the end style for the last point is not squared,
         * or the polyline is closed, output a filled circle
         */
        if ((i < count - 1) || (s_endsty != SQUARED) || closed)
            do_circ(vwk, wx2, wy2);

        /* end point becomes the starting point for the next line segment. */
        wx1 = wx2;
        wy1 = wy2;
    }

    /* Restore the attribute environment. */
    r_fa_attr(vwk);
}


/*
 * draw_arrow - helper function for arrow()
 *
 * performs the actual drawing
 */
static void draw_arrow(Vwk * vwk, Point * point, int count, int inc)
{
    LONG line_len2;
    WORD arrow_len, arrow_wid, line_len;
    WORD dx, dy, dxfactor, dyfactor;
    WORD base_x, base_y, ht_x, ht_y;
    WORD i;
    Point triangle[4];      /* allow room for polygon() to close triangle */
    Point *ptr1, *ptr2, *xybeg;

    line_len2 = dx = dy = 0;

    /* Set up the arrow-head length and width as a function of line width. */
    arrow_len = (vwk->line_width < 4) ? 8 : (3 * vwk->line_width - 1);
    arrow_wid = arrow_len / 2;

    /* Initialize the beginning pointer. */
    ptr1 = ptr2 = point;

    /* Find the first point which is not so close to the end point that it */
    /* will be obscured by the arrowhead.                                  */
    for (i = 1; i < count; i++) {
        /* Find the deltas between the next point and the end point. Transform */
        /* to a space such that the aspect ratio is uniform and the x axis */
        /* distance is preserved. */

        ptr1 += inc;
        dx = ptr2->x - ptr1->x;
        dy = mul_div_round(ptr2->y - ptr1->y, ysize, xsize);

        /* Get length of vector connecting the point with the end point. */
        /* If the vector is of sufficient length, the search is over. */
        line_len2 = (LONG)dx*dx + (LONG)dy*dy;
        if (line_len2 >= (LONG)arrow_len*arrow_len)
            break;
    }                           /* End for:  over i. */
    line_len = Isqrt(line_len2);

    /* Set xybeg to the point we found */
    xybeg = ptr1;

    /* If the longest vector is insufficiently long, don't draw an arrow. */
    if (line_len < arrow_len)
        return;

    /* Rotate the arrow-head height and base vectors.  Perform calculations */
    /* in 1000x space.                                                      */

    dxfactor = mul_div_round(dx, 1000, line_len);
    dyfactor = mul_div_round(dy, 1000, line_len);
    ht_x = mul_div_round(arrow_len, dxfactor, 1000);
    ht_y = mul_div_round(arrow_len, dyfactor, 1000);
    base_x = mul_div_round(arrow_wid, -dyfactor, 1000);
    base_y = mul_div_round(arrow_wid, dxfactor, 1000);

    /* Transform the y offsets back to the correct aspect ratio space. */

    ht_y = mul_div_round(ht_y, xsize, ysize);
    base_y = mul_div_round(base_y, xsize, ysize);

    /* Build a polygon into a local array first */
    ptr1 = triangle;
    ptr2 = point;

    ptr1->x = ptr2->x + base_x - ht_x;
    ptr1->y = ptr2->y + base_y - ht_y;
    ptr1++;
    ptr1->x = ptr2->x - base_x - ht_x;
    ptr1->y = ptr2->y - base_y - ht_y;
    ptr1++;
    ptr1->x = ptr2->x;
    ptr1->y = ptr2->y;

    polygon(vwk, triangle, 3);

    /* Adjust the end point and all points skipped. */
    ptr1 = point;
    ptr2 = xybeg;

    ptr1->x -= ht_x;
    ptr1->y -= ht_y;

    while ((ptr2 -= inc) != ptr1) {
        ptr2->x = ptr1->x;
        ptr2->y = ptr1->y;
    }
}


/*
 * arrow - draw arrow(s) at the end(s) of the line
 *
 * Will alter the end of the line segment.
 */
void arrow(Vwk * vwk, Point * point, int count)
{
    /* Set up the attribute environment. */
    s_fa_attr(vwk);

    /* beginning point is arrowed. */
    if (s_begsty & ARROWED) {
        draw_arrow(vwk, point, count, 1);
    }

    /* ending point is arrowed. */
    if (s_endsty & ARROWED) {
        draw_arrow(vwk, point+count-1, count, -1);
    }

    /* Restore the attribute environment. */
    r_fa_attr(vwk);
}


/*
 * abline - draw a line
 *
 * this is now a wrapper for the actual line drawing routines
 *
 * input:
 *     line         = pointer to structure containing coordinates
 *     v_planes     = number of video planes
 *     LN_MASK      = line mask (for dashed/dotted lines)
 *     LSTLIN       = flag: TRUE iff this is the last line of a polyline
 *     wrt_mode     = writing mode:
 *                          0 => replace mode.
 *                          1 => or mode.
 *                          2 => xor mode.
 *                          3 => not mode.
 *
 * output:
 *     LN_MASK rotated to proper alignment with x coordinate of line end
 */
void abline(const Line *line, const WORD wrt_mode, UWORD color)
{
    Line ordered;
    UWORD x1,y1,x2,y2;          /* the coordinates */

#if CONF_WITH_VDI_VERTLINE
    /*
     * optimize drawing of vertical lines
     */
    if (line->x1 == line->x2) {
#if CONF_WITH_VDI_16BIT
        if (TRUECOLOR_MODE)
        {
            swblit_vertical_line16(line, wrt_mode, color);
            return;
        }
        else
#endif
#if CONF_WITH_BLITTER
        if (blitter_is_enabled)
        {
            hwblit_vertical_line(line, wrt_mode, color);
            return;
        }
        else
#endif
        {
            vertical_line(line, wrt_mode, color);
            return;
        }
    }
#endif

    /* Always draw from left to right */
    if (line->x2 < line->x1) {
        /* if delta x < 0 then draw from point 2 to 1 */
        x1 = line->x2;
        y1 = line->y2;
        x2 = line->x1;
        y2 = line->y1;
    } else {
        /* positive, start with first point */
        x1 = line->x1;
        y1 = line->y1;
        x2 = line->x2;
        y2 = line->y2;
    }

    /*
     * copy a DRI kludge: if we're in XOR mode, avoid XORing intermediate
     * points in a polyline.  we do it slightly differently than DRI with
     * slightly differing results - but it's a kludge in either case.
     */
    if ((wrt_mode == WM_XOR) && !LSTLIN)
    {
        if (x1 != x2)
            x2--;
        else if (y1 != y2)
            y2--;
    }

    /*
     * optimize drawing of horizontal lines
     */
    if (y1 == y2) {
        UWORD linemask = LN_MASK;   /* linestyle bits */
        VwkAttrib attr;
        Rect rect;

        attr.clip = 0;
        attr.multifill = 0;
        attr.patmsk = 0;
        attr.patptr = &linemask;
        attr.wrt_mode = wrt_mode;
        attr.color = color;
        rect.x1 = x1;
        rect.y1 = y1;
        rect.x2 = x2;
        rect.y2 = y2;
        draw_rect_common(&attr, &rect);
        return;
    }

    /*
     * draw any line
     */
    ordered.x1 = x1;
    ordered.y1 = y1;
    ordered.x2 = x2;
    ordered.y2 = y2;
#if CONF_WITH_VDI_16BIT
    if (TRUECOLOR_MODE)
        draw_line16(&ordered, wrt_mode, color);
    else
#endif
    draw_line(&ordered, wrt_mode, color);
}

/*
 * Line-A wrapper for line drawing with abline
 */
void linea_line(void)
{
    Line line;

    line.x1 = X1;
    line.y1 = Y1;
    line.x2 = X2;
    line.y2 = Y2;

    /* Line-A LN_MASK is already set by caller */
    abline(&line, WRT_MODE, linea_color());
}


/*
 * set LN_MASK from virtual workstation values
 */
void set_LN_MASK(Vwk *vwk)
{
    WORD l;

    l = vwk->line_index;
    LN_MASK = (l < 6) ? LINE_STYLE[l] : vwk->ud_ls;
}
