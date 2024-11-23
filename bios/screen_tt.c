/*
 * screen_tt.c - low-level screen routines for the Atari TT
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

static const UWORD tt_dflt_palette[] = {
 TTRGB_WHITE, TTRGB_RED, TTRGB_GREEN, TTRGB_YELLOW,
 TTRGB_BLUE, TTRGB_MAGENTA, TTRGB_CYAN, TTRGB_LTGRAY,
 TTRGB_GRAY, TTRGB_LTRED, TTRGB_LTGREEN, TTRGB_LTYELLOW,
 TTRGB_LTBLUE, TTRGB_LTMAGENTA, TTRGB_LTCYAN, TTRGB_BLACK,
 0x0fff, 0x0eee, 0x0ddd, 0x0ccc, 0x0bbb, 0x0aaa, 0x0999, 0x0888,
 0x0777, 0x0666, 0x0555, 0x0444, 0x0333, 0x0222, 0x0111, 0x0000,
 0x0f00, 0x0f01, 0x0f02, 0x0f03, 0x0f04, 0x0f05, 0x0f06, 0x0f07,
 0x0f08, 0x0f09, 0x0f0a, 0x0f0b, 0x0f0c, 0x0f0d, 0x0f0e, 0x0f0f,
 0x0e0f, 0x0d0f, 0x0c0f, 0x0b0f, 0x0a0f, 0x090f, 0x080f, 0x070f,
 0x060f, 0x050f, 0x040f, 0x030f, 0x020f, 0x010f, 0x000f, 0x001f,
 0x002f, 0x003f, 0x004f, 0x005f, 0x006f, 0x007f, 0x008f, 0x009f,
 0x00af, 0x00bf, 0x00cf, 0x00df, 0x00ef, 0x00ff, 0x00fe, 0x00fd,
 0x00fc, 0x00fb, 0x00fa, 0x00f9, 0x00f8, 0x00f7, 0x00f6, 0x00f5,
 0x00f4, 0x00f3, 0x00f2, 0x00f1, 0x00f0, 0x01f0, 0x02f0, 0x03f0,
 0x04f0, 0x05f0, 0x06f0, 0x07f0, 0x08f0, 0x09f0, 0x0af0, 0x0bf0,
 0x0cf0, 0x0df0, 0x0ef0, 0x0ff0, 0x0fe0, 0x0fd0, 0x0fc0, 0x0fb0,
 0x0fa0, 0x0f90, 0x0f80, 0x0f70, 0x0f60, 0x0f50, 0x0f40, 0x0f30,
 0x0f20, 0x0f10, 0x0b00, 0x0b01, 0x0b02, 0x0b03, 0x0b04, 0x0b05,
 0x0b06, 0x0b07, 0x0b08, 0x0b09, 0x0b0a, 0x0b0b, 0x0a0b, 0x090b,
 0x080b, 0x070b, 0x060b, 0x050b, 0x040b, 0x030b, 0x020b, 0x010b,
 0x000b, 0x001b, 0x002b, 0x003b, 0x004b, 0x005b, 0x006b, 0x007b,
 0x008b, 0x009b, 0x00ab, 0x00bb, 0x00ba, 0x00b9, 0x00b8, 0x00b7,
 0x00b6, 0x00b5, 0x00b4, 0x00b3, 0x00b2, 0x00b1, 0x00b0, 0x01b0,
 0x02b0, 0x03b0, 0x04b0, 0x05b0, 0x06b0, 0x07b0, 0x08b0, 0x09b0,
 0x0ab0, 0x0bb0, 0x0ba0, 0x0b90, 0x0b80, 0x0b70, 0x0b60, 0x0b50,
 0x0b40, 0x0b30, 0x0b20, 0x0b10, 0x0700, 0x0701, 0x0702, 0x0703,
 0x0704, 0x0705, 0x0706, 0x0707, 0x0607, 0x0507, 0x0407, 0x0307,
 0x0207, 0x0107, 0x0007, 0x0017, 0x0027, 0x0037, 0x0047, 0x0057,
 0x0067, 0x0077, 0x0076, 0x0075, 0x0074, 0x0073, 0x0072, 0x0071,
 0x0070, 0x0170, 0x0270, 0x0370, 0x0470, 0x0570, 0x0670, 0x0770,
 0x0760, 0x0750, 0x0740, 0x0730, 0x0720, 0x0710, 0x0400, 0x0401,
 0x0402, 0x0403, 0x0404, 0x0304, 0x0204, 0x0104, 0x0004, 0x0014,
 0x0024, 0x0034, 0x0044, 0x0043, 0x0042, 0x0041, 0x0040, 0x0140,
 0x0240, 0x0340, 0x0440, 0x0430, 0x0420, 0x0410, TTRGB_WHITE, TTRGB_BLACK
};

/*
 * TT shifter functions
 */

/* xbios routines */

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

/*
 * Initialise TT palette
 *
 * Note the following special handling for the TT's "Duochrome" mode,
 * (used when you select "ST High" on the TT desktop):
 *
 * For Duochrome mode, TT palette register 0 contains the inversion bit
 * (bit 1), and the foreground/background colours are in registers 254/255.
 * For both TOS3 and EmuTOS, the initial value for VDI pens 0/254/255 are
 * white/white/black for all resolutions, which causes hardware registers
 * 0/254/255 to be set to 0x0fff/0x0fff/0x000.
 *
 * Without any compensation, this would cause problems when switching to
 * duochrome mode: since the inversion bit in register 0 is set, the display
 * would show as white on black.  Since it's desirable for other reasons to
 * leave register 0 as white, TOS3 (and EmuTOS) compensate as follows: if
 * the inversion bit is set, the values in registers 254/255 are swapped.
 * This produces the correct black on white display.
 */
static void initialise_tt_palette(WORD rez)
{
    int i;

    for (i = 0; i < 256; i++)
        TT_PALETTE_REGS[i] = tt_dflt_palette[i];

    if (rez == TT_HIGH) {
        /* TT_PALETTE_REGS[1] is updated by h/w */
        TT_PALETTE_REGS[1] = TT_PALETTE_REGS[15];
        return;
    }

    /* special handling for Duochrome mode */
    if ((rez == ST_HIGH) && (tt_dflt_palette[0] & TT_DUOCHROME_INVERT)) {
        TT_PALETTE_REGS[254] = tt_dflt_palette[255];
        TT_PALETTE_REGS[255] = tt_dflt_palette[254];
    }
}

#endif /* CONF_WITH_TT_SHIFTER */
