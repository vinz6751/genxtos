/*
 * vdi_inline.h - Definitions of VDI inline functions
 *
 * Copyright 2024-2025 The EmuTOS development team.
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#ifndef _VDI_INLINE_H
#define _VDI_INLINE_H

/*
 * get_start_addr - return memory address for column x, row y
 *
 * NOTE: the input x value may be negative (for example, this happens
 * when handling a slanting wideline starting at pixel 0 of a row).  This
 * value must be right-shifted to obtain an offset in bytes.
 * According to the C standard, the result of right-shifting a negative
 * value is implementation-defined.  GCC has the correct behaviour from
 * our point of view: high-order bits are 1-filled, so the number remains
 * negative.
 */


static __inline__ UWORD *get_start_addr(const WORD x, const WORD y)
{
    UBYTE * addr;

    /* init address counter */
    addr = v_bas_ad;                    /* start of screen */

#if CONF_WITH_CHUNKY8
    addr += x;
#else
    WORD x2 = x & 0xfff0;               /* ensure that value to be shifted remains signed! */
    addr += x2 >> v_planes_shift;       /* add x coordinate part of addr */
#endif

    addr += muls(y, v_lin_wr);          /* add y coordinate part of addr */

    return (UWORD*)addr;
}

#if CONF_WITH_VDI_16BIT
static __inline__ UWORD *get_start_addr16(const WORD x, const WORD y)
{
    return (UWORD *)(v_bas_ad + (x * sizeof(WORD)) + muls(y, v_lin_wr));
}
#endif

#endif                          /* _VDI_INLINE_H */
