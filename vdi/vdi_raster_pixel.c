/*
 * vdi_raster_pixel.c - framebuffer format-specific pixel-based operations
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
#include "vdi_raster_pixel.h"

/*
 * get_color - Get color value of requested pixel.
 */
 UWORD
 get_color (UWORD mask, UWORD * addr)
 {
 #if CONF_WITH_CHUNKY8
     return ((UBYTE*)addr)[mask]; /* mask is really x */
 #else
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
 #endif    
 }


 /*
 * pixelread - gets a pixel's colour
 *
 * For palette-based resolutions, this returns the colour index; for
 * Truecolor resolutions, this returns the 16-bit RGB colour.
 *
 * input:
 *     PTSIN(0) = x coordinate.
 *     PTSIN(1) = y coordinate.
 * output:
 *     pixel colour
 */
UWORD
pixelread(const WORD x, const WORD y)
{
    UWORD *addr;
    UWORD mask;

#if CONF_WITH_VDI_16BIT
    if (TRUECOLOR_MODE)
    {
        addr = get_start_addr16(x, y);
        return *addr;                   /* just return the data at the address */
    }
#endif

    /* convert x,y to start address and bit mask */
    addr = get_start_addr(x, y);

#if CONF_WITH_CHUNKY8
    mask = x;
#else
    addr += v_planes;                   /* start at highest-order bit_plane */
    mask = 0x8000 >> (x&0xf);           /* initial bit position in WORD */
#endif

    return get_color(mask, addr);       /* return the composed color value */
}


/*
 * put_pix - plot a pixel (just for line-A)
 *
 * NOTE: this does not work for Truecolor modes in TOS4 due to a bug.
 * Register a4 is used to reference the lineA pointer table, but has
 * never been set; the code should be using a1 instead.  So we can
 * safely assume that no existing program is expecting this to work.
 *
 * However, because EmuTOS aims to be better than TOS, a functioning
 * Truecolor mode has been implemented.  The EmuTOS Truecolor code
 * is based on what TOS4 apparently intends to do, i.e. just stores
 * the word passed in INTIN[0] as-is.  This also meshes with the
 * operation of linea2 in TOS4 Truecolor modes, which just retrieves
 * the word at the specified address.
 *
 * input:
 *     INTIN(0) = pixel value.
 *     PTSIN(0) = x coordinate.
 *     PTSIN(1) = y coordinate.
 */
 void
 pixelput(const WORD x, const WORD y)
 {
     UWORD *addr;
  
 #if CONF_WITH_VDI_16BIT
     if (TRUECOLOR_MODE)
     {
         /*
          * convert x,y to start address & validate
          */
         addr = get_start_addr16(x, y);
         if (addr < (UWORD*)v_bas_ad || addr >= get_start_addr16(V_REZ_HZ, V_REZ_VT))
             return;
         *addr = INTIN[0];   /* store 16-bit Truecolor value */
         return;
     }
 #endif
 
     /* convert x,y to start address */
     addr = get_start_addr(x, y);
     /* co-ordinates can wrap, but cannot write outside screen,
      * alternatively this could check against v_bas_ad+vram_size()
      */
     if (addr < (UWORD*)v_bas_ad || addr >= get_start_addr(V_REZ_HZ, V_REZ_VT)) {
         return;
     }
 
 #if CONF_WITH_CHUNKY8
     *((UBYTE*)addr) = (UBYTE)(INTIN[0]);
 #else
     UWORD color;
     UWORD mask;
     int plane;
 
     mask = 0x8000 >> (x&0xf);   /* initial bit position in WORD */
     color = INTIN[0];           /* device dependent encoded color bits */
 
     for (plane = v_planes; plane; plane--) {
         if (color&0x0001)
             *addr++ |= mask;
         else
             *addr++ &= ~mask;
         color >>= 1;
     }
 #endif    
 }
 


#if CONF_WITH_VDI_16BIT
/*
 * search_to_right16() - Truecolor version of search_to_right()
 */
UWORD search_to_right16(const VwkClip *clip, WORD x, const UWORD search_col, UWORD *addr)
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
UWORD search_to_left16(const VwkClip *clip, WORD x, const UWORD search_col, UWORD *addr)
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
WORD end_pts16(const VwkClip *clip, WORD x, WORD y, UWORD search_color, BOOL seed_type, WORD *xleftout, WORD *xrightout)
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
#endif



UWORD
search_to_right (const VwkClip * clip, WORD x, UWORD mask, const UWORD search_col, UWORD * addr)
{
    /* is x coord < x resolution ? */
    while( x++ < clip->xmx_clip ) {
        UWORD color;

#if CONF_WITH_CHUNKY8
#else
        /* need to jump over interleaved bit_plane? */
        rorw1(mask);    /* rotate right */
        if ( mask & 0x8000 )
            addr += v_planes;
#endif
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

#if CONF_WITH_CHUNKY8
#else
        /* need to jump over interleaved bit_plane? */
        rolw1(mask);    /* rotate left */
        if ( mask & 0x0001 )
            addr -= v_planes;
#endif

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

#if CONF_WITH_VDI_16BIT
    if (TRUECOLOR_MODE)
    {
        return end_pts16(clip, x, y, search_color,xleftout, xrightout);
    }
#endif

    /* convert x,y to start address and bit mask */
    addr = get_start_addr(x, y);
#if CONF_WITH_CHUNKY8    
#else    
    addr += v_planes;                   /* start at highest-order bit_plane */
    mask = 0x8000 >> (x & 0x000f);   /* fetch the pixel mask. */
#endif

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
