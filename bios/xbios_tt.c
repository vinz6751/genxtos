/*
 * xbios_tt.c - XBIOS screen routines for the Atari TT
 *
 * Copyright (C) 2001-2024 The EmuTOS development team
 *
 * Authors:
 *  MAD   Martin Doering
 *  THH   Thomas Huth
 *  LVL   Laurent Vogel
 *  PES   Petr Stehlik
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#include "emutos.h"

#if CONF_WITH_TT_SHIFTER

#include "has.h"
#include "screen.h"
#include "screen_tt.h"
#include "xbios_tt.h"

/*
 * Set TT shifter mode
 */
WORD esetshift(WORD mode)
{
    volatile WORD *resreg = (WORD *)TT_SHIFTER;
    WORD oldmode;

    if (!has_tt_shifter)
        return 0x50;    /* unimplemented xbios call: return function # */

    /*
     * to avoid a possible resolution change in the middle of a screen
     * display, we wait for a VBL (TOS3 does this too)
     */
    vsync();

    oldmode = *resreg & TT_SHIFTER_BITMASK;
    *resreg = mode & TT_SHIFTER_BITMASK;

    /*
     * because the resolution may have changed, we must reinitialise
     * the screen services (Line-A, VT52 emulator)
     */
    screen_init_services();

    return oldmode;
}


/*
 * Get TT shifter mode
 */
WORD egetshift(void)
{
    if (!has_tt_shifter)
        return 0x51;    /* unimplemented xbios call: return function # */

    return *(volatile WORD *)TT_SHIFTER & TT_SHIFTER_BITMASK;
}


/*
 * Read/modify TT shifter colour bank number
 */
WORD esetbank(WORD bank)
{
    volatile UBYTE *shiftreg = (UBYTE *)(TT_SHIFTER+1);
    UBYTE old;

    if (!has_tt_shifter)
        return 0x52;    /* unimplemented xbios call: return function # */

    old = *shiftreg & 0x0f;
    if (bank >= 0)
        *shiftreg = bank & 0x0f;

    return old;
}


/*
 * Read/modify TT palette colour entry
 */
WORD esetcolor(WORD index,UWORD color)
{
    UWORD oldcolor;

    if (!has_tt_shifter)
        return 0x53;    /* unimplemented xbios call: return function # */

    index &= 0xff;                  /* force valid index number */
    oldcolor = TT_PALETTE_REGS[index] & TT_PALETTE_BITMASK;
    if ((WORD)color >= 0)
        TT_PALETTE_REGS[index] = color & TT_PALETTE_BITMASK;

    return oldcolor;
}


/*
 * Set multiple TT palette colour registers
 *
 * This function is defined by Atari to return void; however, if the TT
 * shifter is not present, it should return the function number in a WORD,
 * which is the de facto TOS standard for unimplemented xbios functions.
 * Therefore internally we make it return a WORD.
 */
WORD esetpalette(WORD index,WORD count,UWORD *rgb)
{
    volatile UWORD *ttcolour;

    if (!has_tt_shifter)
        return 0x54;    /* unimplemented xbios call: return function # */

    index &= 0xff;              /* force valid index number */

    if ((index+count) > 256)
        count = 256 - index;    /* force valid count */

    ttcolour = &TT_PALETTE_REGS[index];
    while(count--)
        *ttcolour++ = *rgb++ & TT_PALETTE_BITMASK;

    return 0;
}


/*
 * Get multiple TT palette colour registers
 *
 * See the comments for esetpalette() above
 */
WORD egetpalette(WORD index,WORD count,UWORD *rgb)
{
    volatile UWORD *ttcolour;

    if (!has_tt_shifter)
        return 0x55;    /* unimplemented xbios call: return function # */

    index &= 0xff;              /* force valid index number */

    if ((index+count) > 256)
        count = 256 - index;    /* force valid count */

    ttcolour = &TT_PALETTE_REGS[index];
    while(count--)
        *rgb++ = *ttcolour++ & TT_PALETTE_BITMASK;

    return 0;
}


/*
 * Read/modify TT shifter grey mode bit
 */
WORD esetgray(WORD mode)
{
    volatile UBYTE *shiftreg = (UBYTE *)TT_SHIFTER;
    UBYTE old;

    if (!has_tt_shifter)
        return 0x56;    /* unimplemented xbios call: return function # */

    old = *shiftreg;
    if (mode > 0)
        *shiftreg = old | 0x10;
    else if (mode == 0)
        *shiftreg = old & 0xef;

    return (old&0x10)?1:0;
}


/*
 * Read/modify TT shifter smear mode bit
 */
WORD esetsmear(WORD mode)
{
    volatile UBYTE *shiftreg = (UBYTE *)TT_SHIFTER;
    UBYTE old;

    if (!has_tt_shifter)
        return 0x57;    /* unimplemented xbios call: return function # */

    old = *shiftreg;
    if (mode > 0)
        *shiftreg = old | 0x80;
    else if (mode == 0)
        *shiftreg = old & 0x7f;

    return (old&0x80)?1:0;
}

#endif /* CONF_WITH_TT_SHIFTER */