/*
 * vdi_textblit.c - the text_blt() mainline code
 *
 * Copyright (C) 2017-2025 The EmuTOS development team
 *
 * Authors:
 *  RFB   Roger Burrows
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

/* #define ENABLE_KDEBUG */

#include "emutos.h"
#include "asm.h"
#include "intmath.h"

#include "tosvars.h"
#include "vdi_defs.h"
#include "vdistub.h"
#include "lineavars.h"
#include "vdi_inline.h"
#include "vdi_textblit.h"
#include "vdi_raster_driver.h"
#include "biosext.h"

/* here we should have the preprocessor verify the length of LOCALVARS */

/*
 * the following table maps a 4-bit sequence to its reverse
 */
static UBYTE reverse_nybble[] =
/*  0000  0001  0010  0011  0100  0101  0110  0111  */
{   0x00, 0x08, 0x04, 0x0c, 0x02, 0x0a, 0x06, 0x0e,
/*  1000  1001  1010  1011  1100  1101  1110  1111  */
    0x01, 0x09, 0x05, 0x0d, 0x03, 0x0b, 0x07, 0x0f };


/*
 * check for clipping
 *
 * returns  1 clipping required
 *          0 no clipping
 *         -1 entirely clipped, nothing to output
 */
static WORD check_clip(LOCALVARS *vars, WORD delx, WORD dely)
{
    WORD rc;

    if (!CLIP)
        return 0;

    rc = 0;

    /*
     * check x coordinate
     */
    if (vars->DESTX < XMINCL)               /* (partially) left of clip window */
    {
        if (vars->DESTX+delx <= XMINCL)     /* wholly left of clip window */
            return -1;
        rc = 1;
    }
    if (vars->DESTX > XMAXCL)               /* wholly right of clip window */
        return -1;
    if (vars->DESTX+delx > XMAXCL)          /* partially right of clip window */
        rc = 1;

    /*
     * check y coordinate
     */
    if (vars->DESTY < YMINCL)               /* (partially) below clip window */
    {
        if (vars->DESTY+dely <= YMINCL)     /* wholly below clip window */
            return -1;
        rc = 1;
    }
    if (vars->DESTY > YMAXCL)               /* wholly above clip window */
        return -1;
    if (vars->DESTY+dely > YMAXCL)          /* partially above clip window */
        rc = 1;

    return rc;
}


/*
 * do the actual clipping
 *
 * returns  0 OK
 *         -1 entirely clipped, nothing to output.  I believe this can
 *            only happen if rotation or scaling has been done
 */
static WORD do_clip(LOCALVARS *vars)
{
    WORD n;

    /*
     * if clipping not requested, exit
     */
    if (!CLIP)
        return 0;

    /*
     * clip x minimum if necessary
     */
    if (vars->DESTX < XMINCL)
    {
        n = vars->DESTX + vars->DELX - XMINCL;
        if (n <= 0)
            return -1;
        SOURCEX += vars->DELX - n;
        vars->DELX = n;
        vars->DESTX = XMINCL;
    }

    /*
     * clip x maximum if necessary
     */
    if (vars->DESTX > XMAXCL)
        return -1;
    n = vars->DESTX + vars->DELX - XMAXCL - 1;
    if (n > 0)
        vars->DELX -= n;

    /*
     * clip y minimum if necessary
     */
    if (vars->DESTY < YMINCL)
    {
        n = vars->DESTY + vars->DELY - YMINCL;
        if (n <= 0)
            return -1;
        SOURCEY += vars->DELY - n;
        vars->DELY = n;
        vars->DESTY = YMINCL;
    }

    /*
     * clip y maximum if necessary
     */
    if (vars->DESTY > YMAXCL)
        return -1;
    n = vars->DESTY + vars->DELY - YMAXCL - 1;
    if (n > 0)
        vars->DELY -= n;

    return 0;
}


/*
 * return a ULONG equal to the 3 high-order bytes pointed to by
 * the input pointer, concatenated with the low-order byte of
 * the input UWORD
 */
static ULONG merge_byte(UWORD *p, UWORD n)
{
    union {
        ULONG a;
        UBYTE b[4];
    } work;

    work.a = *(ULONG *)p;
    work.b[3] = n;

    return work.a;
}


/*
 * outline: perform text outlining
 *
 * in the following code, the top 18 bits of unsigned longs are used
 * to manage 18-bit values, consisting of the last bit of the previous
 * screen word, the 16 bits of the current screen word, and the first
 * bit of the next screen word.
 *
 * neighbours are numbered as follows:
 *              1 2 3
 *              7 X 8
 *              4 5 6
 */
void outline(LOCALVARS *vars)
{
    UWORD *currline, *nextline, *scratch;
    UWORD *save_next;
    UWORD curr, prev, tmp;
    WORD i, j, form_width;
    ULONG current_left, current_noshift, current_right;
    ULONG top_left, top_noshift, top_right;
    ULONG bottom_left, bottom_noshift, bottom_right;
    ULONG result;

    form_width = vars->s_next / sizeof(WORD);
    currline = (UWORD *)vars->sform + form_width;
    nextline = currline + form_width;

    /* process lines sequentially */
    for (i = vars->DELY; i > 0; i--)
    {
        save_next = nextline;
        prev = 0;
        curr = 0;
        bottom_left = (*(ULONG *)nextline) >> 1;

        /* process one word at a time */
        for (j = form_width, scratch = (UWORD *)vars->sform; j > 0; j--)
        {
            /* get the data from the current line */
            current_left = current_noshift = merge_byte(currline, curr);
            rorl(current_noshift, 1);
            current_right = current_noshift;
            rorl(current_right, 1);

            /*
             * get the data from the scratch buffer (the previous line)
             * and merge into result
             */
            top_right = merge_byte(scratch, prev);
            rorl(top_right, 1);
            top_left = top_noshift = top_right;
            top_left ^= current_left;           /* neighbours 1,2,3 */
            top_noshift ^= current_noshift;
            top_right ^= current_right;
            roll(top_noshift, 1);
            roll(top_right, 2);
            result = (top_left | top_noshift | top_right);

            /* get the data from the next line & merge into result */
            bottom_right = bottom_noshift = bottom_left;
            bottom_left ^= current_left;        /* neighbours 4,5,6 */
            bottom_noshift ^= current_noshift;
            bottom_right ^= current_right;
            roll(bottom_noshift, 1);
            roll(bottom_right, 2);
            result |= (bottom_left | bottom_noshift | bottom_right);

            /* finally, merge current line neighbours */
            current_left ^= current_noshift;    /* neighbours 7,8 */
            current_right ^= current_noshift;
            roll(current_right, 2);
            result |= (current_left | current_right);
            result >>= 16;                      /* move to lower 16 bits */

            prev = curr = *currline;
            prev = (prev ^ result) & result;
            *currline++ = prev;
            prev = *scratch;
            *scratch++ = curr;

            tmp = *nextline++;
            bottom_left = merge_byte(nextline, tmp);
            rorl(bottom_left, 1);
        }

        nextline = save_next;
        currline = nextline;

        if (i > 2)                              /* mustn't go past end */
            nextline += form_width;
    }
}


/*
 * copy the character into a buffer so that we can
 * outline/rotate/scale it
 */
static void pre_blit(LOCALVARS *vars)
{
    WORD weight, skew, size, n, tmp_style;
    WORD dest_width, dest_height;
    WORD *p;
    LONG offset;
    UBYTE *src, *dst;

    vars->height = vars->DELY;

    vars->tsdad = SOURCEX & 0x000f;     /* source dot address */
    offset = (SOURCEY+vars->DELY-1) * (LONG)vars->s_next + ((SOURCEX >> 3) & ~1);
    src = vars->sform + offset;         /* bottom of font char source */
    vars->s_next = -vars->s_next;       /* we draw from the bottom up */

    weight = WEIGHT;
    skew = LOFF + ROFF;

    dest_width = vars->DELX;
    if (vars->STYLE & F_THICKEN)
    {
        dest_width += weight;
        vars->smear = weight;
    }

    /*
     * handle outlining
     */
    vars->tddad = 0;
    dest_height = vars->DELY;
    if (vars->STYLE & F_OUTLINE)
    {
        dest_width += 3;        /* add 1 left & 2 right pixels */
        vars->tddad += 1;       /* and make leftmost column blank */
        vars->DELY += 2;        /* add 2 rows */
        dest_height += 3;       /* add 3 rows for buffer clear */
    }
    vars->width = dest_width;
    dest_width += skew;
    vars->DELX = dest_width;
    dest_width = ((dest_width >> 4) << 1) + 2;    /* in bytes */
    vars->d_next = -dest_width;
    size = dest_width * (dest_height - 1);
    vars->buffa = SCRPT2 - vars->buffa; /* switch buffers */
    dst = (UBYTE *)SCRTCHP + vars->buffa;
    vars->sform = dst;
    if (vars->STYLE & (F_OUTLINE|F_SKEW))
    {
        p = (WORD *)vars->sform;
        n = (size - vars->d_next) / 2;  /* add bottom line */
        while(n--)
            *p++ = 0;               /* clear buffer */
        if (vars->STYLE & F_OUTLINE)
        {
            vars->width -= 3;
            vars->DELX -= 1;
            size += vars->d_next;
        }
    }

    dst += size;                     /* start at the bottom */
    vars->WRT_MODE = 0;
    vars->forecol = 1;
    vars->ambient = 0;
    vars->nbrplane = 1;             /* 1 plane for this blit */
    vars->nextwrd = 2;
    tmp_style = vars->STYLE;        /* save temporarily */
    vars->STYLE &= (F_SKEW|F_THICKEN);  /* only thicken, skew */

    normal_blit(vars+1, src, dst);  /* call assembler helper function */

    vars->STYLE = tmp_style;        /* restore */
    vars->WRT_MODE = WRT_MODE;
    vars->s_next = -vars->d_next;   /* reset the source to the buffer */

    if (vars->STYLE & F_OUTLINE)
    {
        outline(vars);
        vars->sform += vars->s_next;
    }

    SOURCEX = 0;
    SOURCEY = 0;
    vars->STYLE &= ~(F_SKEW|F_THICKEN); /* cancel effects */
}


/*
 * rotate: perform text rotation
 */
void rotate(LOCALVARS *vars)
{
    UBYTE *src, *dst;
    WORD form_width, i, j, k, tmp;
    UWORD in, out, srcbit, dstbit;
    UWORD *p, *q;

    vars->tsdad = SOURCEX & 0x000f;
    src = vars->sform + ((SOURCEX >> 4) << 1);
    vars->buffa = SCRPT2 - vars->buffa;     /* switch buffers */
    dst = (UBYTE *)SCRTCHP + vars->buffa;

    vars->width = vars->DELX;
    vars->height = vars->DELY;

    /*
     * first, handle the simplest case: inverted text (180 rotation)
     */
    if (CHUP == 1800)
    {
        form_width = ((vars->DELX+vars->tsdad-1) >> 4) + 1; /* in words */
        vars->d_next = form_width * sizeof(WORD);
        q = (UWORD *)(dst + vars->d_next * vars->height);
        for (i = vars->height; i > 0; i--)
        {
            for (j = form_width, p = (UWORD *)src; j > 0; j--)
            {
                in = *p++;
                out = reverse_nybble[in&0x000f];    /* reverse 4 bits at a time */
                for (k = 3; k > 0; k--)
                {
                    out <<= 4;
                    in >>= 4;
                    out |= reverse_nybble[in&0x000f];
                }
                *--q = out;
            }
            src += vars->s_next;
        }
        vars->s_next = vars->d_next;
        vars->sform = (UBYTE *)SCRTCHP + vars->buffa;
        SOURCEX = -(SOURCEX + vars->DELX) & 0x000f;
        SOURCEY = 0;
        return;
    }

    /*
     * handle remaining cases (90 and 270 rotation)
     */
    vars->d_next = ((vars->DELY >> 4) << 1) + 2;

    if (CHUP == 900)
    {
        dst += (vars->DELX - 1) * vars->d_next;
        vars->d_next = -vars->d_next;
    }
    else        /* 2700 */
    {
        src += (SOURCEY + vars->height - 1) * vars->s_next;
        vars->s_next = -vars->s_next;
    }

    srcbit = 0x8000 >> vars->tsdad;
    dstbit = 0x8000;
    out = 0;
    p = (UWORD *)src;
    q = (UWORD *)dst;
    for (i = vars->width; i > 0; i--)
    {
        for (j = vars->height; j > 0; j--)
        {
            if (*p & srcbit)
                out |= dstbit;
            dstbit >>= 1;
            if (!dstbit)
            {
                dstbit = 0x8000;
                *q++ = out;
                out = 0;
            }
            p = (UWORD *)((UBYTE *)p + vars->s_next);
        }

        dstbit = 0x8000;
        *q = out;
        out = 0;
        dst += vars->d_next;
        q = (UWORD *)dst;

        srcbit >>= 1;
        if (!srcbit)
        {
            srcbit = 0x8000;
            src += sizeof(WORD);
        }

        p = (UWORD *)src;
    }

    vars->height = vars->DELX;  /* swap width & height */
    vars->width = vars->DELY;
    vars->DELX = vars->width;
    vars->DELY = vars->height;
    tmp = vars->tmp_delx;
    vars->tmp_delx = vars->tmp_dely;
    vars->tmp_dely = tmp;
    vars->swap_tmps = 1;

    vars->s_next = (CHUP == 900) ? -vars->d_next : vars->d_next;
    vars->sform = (UBYTE*)SCRTCHP + vars->buffa;
    SOURCEX = 0;
    SOURCEY = 0;
}


/*
 * inline function to clarify horizontal scaling code
 */
static __inline__ UWORD *shift_and_update(UWORD *dst, UWORD *dstbit, UWORD *out)
{
    *dstbit >>= 1;
    if (!*dstbit)           /* end of word ? */
    {
        *dstbit = 0x8000;   /* reset test bit */
        *dst++ = *out;      /* output accumulated word */
        *out = 0;           /* & reset it */
    }

    return dst;
}


/*
 * scaleup: increase width of character (SCALDIR is 1)
 */
static void scaleup(LOCALVARS *vars, UWORD *src, UWORD *dst)
{
    UWORD srcbit, dstbit;
    UWORD accum, in, out;
    WORD i;

    srcbit = 0x8000 >> vars->tsdad;
    dstbit = 0x8000;

    out = 0;
    accum = XDDA;
    in = *src++;        /* prime the source word */

    for (i = vars->width; i > 0; i--)
    {
        if (in & srcbit)            /* handle bit set in source */
        {
            accum += DDAINC;
            if (accum < DDAINC)
            {
                out |= dstbit;
                dst = shift_and_update(dst, &dstbit, &out);
            }
            out |= dstbit;
            dst = shift_and_update(dst, &dstbit, &out);
        }
        else                        /* handle bit clear in source */
        {
            accum += DDAINC;
            if (accum < DDAINC)
            {
                dst = shift_and_update(dst, &dstbit, &out);
            }
            dst = shift_and_update(dst, &dstbit, &out);
        }

        srcbit >>= 1;
        if (!srcbit)
        {
            srcbit = 0x8000;
            in = *src++;
        }
    }

    *dst = out;
}


/*
 * scaledown: decrease width of character (SCALDIR is 0)
 */
static void scaledown(LOCALVARS *vars, UWORD *src, UWORD *dst)
{
    UWORD srcbit, dstbit;
    UWORD accum, in, out;
    WORD i;

    srcbit = 0x8000 >> vars->tsdad;
    dstbit = 0x8000;

    out = 0;
    accum = XDDA;
    in = *src++;        /* prime the source word */

    for (i = vars->width; i > 0; i--)
    {
        if (in & srcbit)            /* handle bit set in source */
        {
            accum += DDAINC;
            if (accum < DDAINC)
            {
                out |= dstbit;
                dst = shift_and_update(dst, &dstbit, &out);
            }
        }
        else                        /* handle bit clear in source */
        {
            dst = shift_and_update(dst, &dstbit, &out);
        }

        srcbit >>= 1;
        if (!srcbit)
        {
            srcbit = 0x8000;
            in = *src++;
        }
    }

    *dst = out;
}


/*
 * scale: perform text scaling
 */
void scale(LOCALVARS *vars)
{
    UBYTE *src, *dst;
    WORD i, delx;
    UWORD accum;

    vars->tsdad = SOURCEX & 0x000f;
    src = vars->sform + ((SOURCEX >> 4) << 1) + (SOURCEY * vars->s_next);

    vars->buffa = SCRPT2 - vars->buffa;     /* switch buffers */
    dst = (UBYTE *)SCRTCHP + vars->buffa;

    vars->width = vars->DELX;
    vars->height = vars->DELY;

    vars->d_next = ((vars->width >> 3) << 1) + 2;

    /*
     * first, scale the character
     */
    accum = 0x7fff;
    if (SCALDIR)        /* scale up */
    {
        for (i = vars->height; i > 0; i--)
        {
            accum += DDAINC;
            if (accum < DDAINC)
            {
                scaleup(vars, (UWORD *)src, (UWORD *)dst);
                dst += vars->d_next;
            }
            scaleup(vars, (UWORD *)src, (UWORD *)dst);
            dst += vars->d_next;
            src += vars->s_next;
        }
    }
    else                /* scale down */
    {
        for (i = vars->height; i > 0; i--)
        {
            accum += DDAINC;
            if (accum < DDAINC)
            {
                scaledown(vars, (UWORD *)src, (UWORD *)dst);
                dst += vars->d_next;
            }
            src += vars->s_next;
        }
    }

    /*
     * then, adjust the character spacing
     */
    accum = XDDA;
    delx = SCALDIR ? vars->DELX : 0;
    for (i = vars->DELX; i > 0; i--)
    {
        accum += DDAINC;
        if (accum < DDAINC)
            delx++;
    }
    XDDA = accum;

    vars->DELX = delx;
    vars->DELY = vars->tmp_dely;
    vars->s_next = vars->d_next;
    vars->sform = (UBYTE *)SCRTCHP + vars->buffa;
    SOURCEX = 0;
    SOURCEY = 0;
}




/*
 * resize characters for line-A
 *
 * this is similar to act_siz(), but note that act_siz() always starts
 * with the same value in the accumulator, while here the initial value
 * is passed as an argument.
 *
 * this allows us to use the same routine for scaling height (which is
 * constant for a given string) and width (which can vary since we do a
 * kind of nano-justification).
 *
 * entry:
 *  init        initial value of accumulator
 *  size        value to scale
 *
 * used variables:
 *  DDAINC      DDA increment passed externally
 *  SCALDIR     0 if scale down, 1 if enlarge
 *
 * exit:
 *  new size
 */
static UWORD char_resize(UWORD init, UWORD size)
{
    UWORD accu, retval, i;

    if (DDAINC == 0xffff) {     /* double size */
        return (size<<1);
    }
    accu = init;
    retval = SCALDIR ? size : 0;

    for (i = 0; i < size; i++) {
        accu += DDAINC;
        if (accu < DDAINC)
            retval++;
    }

    /* if input is non-zero, make return value at least 1 */
    if (size && !retval)
        retval = 1;

    return retval;
}


void text_blt(void)
{
    LOCALVARS vars;
    WORD clipped, delx, dely, weight;
    WORD temp;
    BOOL need_preblit = FALSE;

    vars.swap_tmps = 0;

    /*
     * make local copies, just as the original code does
     */
    vars.STYLE = STYLE;
    vars.WRT_MODE = WRT_MODE;
    vars.DELX = DELX;
    vars.DESTX = DESTX;
    vars.DELY = DELY;
    vars.DESTY = DESTY;

    vars.buffa = 0;
    dely = vars.DELY;
    delx = vars.DELX;

    if (SCALE)
    {
        vars.tmp_dely = dely = char_resize(0x7fff, vars.DELY);
        vars.tmp_delx = delx = char_resize(XDDA, vars.DELX);
    }

    vars.smear = 0;
    if (vars.STYLE & F_THICKEN)
    {
        weight = WEIGHT;
        if (weight == 0)        /* cancel thicken if no weight */
            vars.STYLE &= ~F_THICKEN;
        if (!MONO)
            delx += weight;
    }

    if (vars.STYLE & F_SKEW)
    {
        delx += LOFF + ROFF;
    }

    if (vars.STYLE & F_OUTLINE)
    {
        delx += OUTLINE_THICKNESS * 2;
        dely += OUTLINE_THICKNESS * 2;
    }

    switch(CHUP) {
    case 900:
        vars.DESTY -= delx;
        FALLTHROUGH;
    case 2700:
        temp = delx;        /* swap delx/dely for 90 or 270 */
        delx = dely;
        dely = temp;
        break;
    case 1800:
        vars.DESTX -= delx;
    }

    clipped = check_clip(&vars, delx, dely);
    if (clipped < 0)
        goto upda_dst;

    vars.dest_wrd = 0;
    vars.s_next = FWIDTH;
    vars.sform = (UBYTE *)FBASE;

    if (SCALE)
    {
        scale(&vars);
    }

    /*
     * decide if we need to copy the source glyph to a temporary buffer
     * so we can manipulate it before the actual screen blit
     *
     * we copy in the following situations:
     *  (in 16-bit mode) if (skewing OR thickening OR outlining), OR
     *  if outlining, OR
     *     rotating AND (skewing OR thickening), OR
     *     skewing AND clipping-is-required,
     *      call pre_blit()
     */
#if CONF_WITH_VDI_16BIT
    if (TRUECOLOR_MODE && (vars.STYLE & (F_SKEW|F_THICKEN|F_OUTLINE)))
        need_preblit = TRUE;
    else
#endif
    if (vars.STYLE & F_OUTLINE)
        need_preblit = TRUE;
    else if (CHUP && (vars.STYLE & (F_SKEW|F_THICKEN)))
        need_preblit = TRUE;
    else if ((vars.STYLE & F_SKEW) && clipped)
        need_preblit = TRUE;

    if (need_preblit)
    {
        pre_blit(&vars);
    }

    if (CHUP)
    {
        rotate(&vars);
    }

    if (vars.STYLE & F_THICKEN)
    {
        vars.smear = WEIGHT;
        if (!MONO)
            vars.DELX += vars.smear;
    }

    if (do_clip(&vars) == 0)
    {
        /*
         * Bitplane: include body fragment here (hot path).  Other formats
         * use the vtable.
         */
#if CONF_WITH_VDI_16BIT
        if (TRUECOLOR_MODE)
            vdi_raster->blit_glyph(&vars);
        else
#endif
#if CONF_WITH_CHUNKY8
        if (v_planes == 8)
            vdi_raster->blit_glyph(&vars);
        else
#endif
        {
            LOCALVARS *bitplanes_vars = &vars;
#include "raster_driver_atari_bitplanes_blit_glyph_body.c"
        }
    }

upda_dst:
    delx = DELX;
    if (SCALE)
    {
        delx = vars.swap_tmps ? vars.tmp_dely : vars.tmp_delx;
    }

    if (STYLE & F_OUTLINE)
    {
        delx += OUTLINE_THICKNESS * 2;
    }

    if ((STYLE & F_THICKEN) && !MONO)
    {
        delx += WEIGHT;
    }

    switch(CHUP) {
    default:        /* normally 0, the default */
        DESTX += delx;      /* move right by DELX */
        break;
    case 900:
        DESTY -= delx;      /* move up by DELX */
        break;
    case 1800:
        DESTX -= delx;      /* move left by DELX */
        break;
    case 2700:
        DESTY += delx;      /* move down by DELX */
        break;
    }
}
