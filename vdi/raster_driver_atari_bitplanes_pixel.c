/*
 * raster_driver_atari_bitplanes_pixel.c - pixel operations for Atari bitplanes
 *
 * Part of the interleaved-bitplane implementation of VDI_RASTER_DRIVER;
 * see raster_driver_atari_bitplanes.c and vdi_raster_driver.h.
 *
 * In an interleaved-bitplane framebuffer a single pixel is spread over
 * v_planes consecutive WORDs, one bit per plane, so reading or writing a
 * pixel means walking those planes.  That is why the flood-fill probing
 * code lives here too: it inspects pixels one at a time.
 *
 * Copyright 1982 by Digital Research Inc.  All rights reserved.
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
#include "vdi_defs.h"
#include "vdistub.h"
#include "tosvars.h"
#include "lineavars.h"
#include "vdi_inline.h"
#include "raster_driver_atari_bitplanes.h"

/*
 * get_color - Get color value of requested pixel.
 *
 * the planes are walked from the highest-order one downwards, composing
 * the colour index one bit at a time.
 */
UWORD get_color(UWORD mask, UWORD *addr)
{
    UWORD color = 0;                    /* clear the pixel value accumulator. */
    WORD plane = v_planes;

    while(1) {
        /* test the bit. */
        if ( *--addr & mask )
            color |= 1;         /* if 1, set color accumulator bit. */

        if ( --plane == 0 )
            break;

        color <<= 1;            /* shift accumulator for next bit_plane. */
    }

    return color;       /* this is the color we are searching for */
}


/*
 * pixelread - gets a pixel's colour index
 */
UWORD pixelread(WORD x, WORD y)
{
    UWORD *addr;
    UWORD mask;

    /* convert x,y to start address and bit mask */
    addr = get_start_addr(x, y);
    addr += v_planes;                   /* start at highest-order bit_plane */
    mask = 0x8000 >> (x&0xf);           /* initial bit position in WORD */

    return get_color(mask, addr);       /* return the composed color value */
}


/*
 * pixelput - plot a pixel
 *
 * 'color' is the device dependent encoded colour, i.e. one bit per plane.
 */
void pixelput(WORD x, WORD y, UWORD color)
{
    UWORD *addr;
    UWORD mask;
    int plane;

    /* convert x,y to start address */
    addr = get_start_addr(x, y);

    /*
     * co-ordinates can wrap, but cannot write outside screen,
     * alternatively this could check against v_bas_ad+vram_size()
     */
    if (addr < (UWORD*)v_bas_ad || addr >= get_start_addr(V_REZ_HZ, V_REZ_VT))
        return;

    mask = 0x8000 >> (x&0xf);   /* initial bit position in WORD */

    for (plane = v_planes; plane; plane--) {
        if (color&0x0001)
            *addr++ |= mask;
        else
            *addr++ &= ~mask;
        color >>= 1;
    }
}


UWORD
search_to_right (const VwkClip * clip, WORD x, UWORD mask, const UWORD search_col, UWORD * addr)
{
    /* is x coord < x resolution ? */
    while( x++ < clip->xmx_clip ) {
        UWORD color;

        /* need to jump over interleaved bit_plane? */
        rorw1(mask);    /* rotate right */
        if ( mask & 0x8000 )
            addr += v_planes;

        /* search, while pixel color != search color */
        color = get_color(mask, addr);
        if ( search_col != color ) {
            break;
        }

    }

    return x - 1;       /* output x coord -1 to endxright. */
}



UWORD
search_to_left (const VwkClip * clip, WORD x, UWORD mask, const UWORD search_col, UWORD * addr)
{
    /* Now, search to the left. */
    while (x-- > clip->xmn_clip) {
        UWORD color;

        /* need to jump over interleaved bit_plane? */
        rolw1(mask);    /* rotate left */
        if ( mask & 0x0001 )
            addr -= v_planes;

        /* search, while pixel color != search color */
        color = get_color(mask, addr);
        if ( search_col != color )
            break;

    }

    return x + 1;       /* output x coord + 1 to endxleft. */
}



/*
 * end_pts - find the endpoints of a section of solid color
 *           (for the contourfill() routine.)
 *
 * input:   clip        ptr to clipping rectangle
 *          x           starting x value
 *          y           y coordinate of line
 *
 * output:  xleftout    ptr to variable to receive leftmost point of this colour
 *          xrightout   ptr to variable to receive rightmost point of this colour
 *
 * returns success flag:
 *          0 => no endpoints or starting x value on edge
 *          1 => endpoints found
 */
WORD
end_pts(const VwkClip *clip, WORD x, WORD y, UWORD search_color, BOOL seed_type, WORD *xleftout, WORD *xrightout)
{
    UWORD color;
    UWORD * addr;
    UWORD mask;

    /* see, if we are in the y clipping range */
    if ( y < clip->ymn_clip || y > clip->ymx_clip)
        return 0;

    /* convert x,y to start address and bit mask */
    addr = get_start_addr(x, y);
    addr += v_planes;                   /* start at highest-order bit_plane */
    mask = 0x8000 >> (x & 0x000f);   /* fetch the pixel mask. */

    /* get search color and the left and right end */
    color = get_color (mask, addr);
    *xrightout = search_to_right (clip, x, mask, color, addr);
    *xleftout = search_to_left (clip, x, mask, color, addr);

    /* see, if the whole found segment is of search color? */
    if ( color != search_color ) {
        return seed_type ^ 1;   /* return segment not of search color */
    }
    return seed_type ^ 0;       /* return segment is of search color */
}
