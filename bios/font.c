/*
 * font.c - bios part of font loading and initialization
 *
 * Copyright (C) 2004-2014 by Authors (see below)
 * Copyright (C) 2016-2020 The EmuTOS development team
 *
 * Authors:
 *  MAD  Martin Doering
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

/* #define ENABLE_KDEBUG */

#include "emutos.h"
#include "font.h"
#include "country.h"
#include "string.h"
#include "lineavars.h"

/* RAM-copies of the ROM-fontheaders */
Fonthead fon8x16;
Fonthead fon8x8;
Fonthead fon6x6;

/* system font array used by linea0 */
const Fonthead *sysfonts[4];    /* all three fonts and NULL */

/*
 * font_init - set default font to linea, font ring initialization
 */

void font_init(void)
{
    const Fonthead *f6x6, *f8x8, *f8x16;

    /* ask country.c for the right fonts */

    get_fonts(&f6x6, &f8x8, &f8x16);

    /* copy the ROM-fontheaders of 3 system fonts to RAM */

    fon6x6 = *f6x6;
    fon8x8 = *f8x8;
    fon8x16 = *f8x16;

    /*
     * now in RAM, chain only the 8x8 & 8x16 font headers in a linked list.
     * this is how Atari TOS does it, and is required for text_init()
     * [in vdi_text.c] to set up font_ring[] correctly, and subsequently
     * calculate DEV_TAB[5] and font_count correctly.
     */
    fon8x8.next_font = &fon8x16;
    font_count = 1;     /* number of different font ids in font_ring[] */

    /* Initialize the system font array for linea */

    sysfonts[0] = f6x6;
    sysfonts[1] = f8x8;
    sysfonts[2] = f8x16;
    sysfonts[3] = NULL;
}

/*
 * font_set_default
 *
 * choose default font depending on screen height, and set
 * linea variables according to chosen font configuration
 */

Fonthead* font_set_default(void)
{
    Fonthead *font;

#if CONF_WITH_FORCE_8x8_FONT/* Some problems with 8x16 because of cursor so use 8x8 for now */
    font = &fon8x8;
#else
    font = (V_REZ_VT < 400) ? &fon8x8 : &fon8x16;
#endif

    v_cel_ht = font->form_height;
    v_fnt_wr = font->form_width;
    v_fnt_st = font->first_ade;
    v_fnt_nd = font->last_ade;
    v_fnt_ad = font->dat_table;
    v_off_ad = font->off_table;

    return font;
}


/*
 * char_addr - retrieve the address of the source cell in the font
 *
 *
 * Given an offset value.
 *
 * in:
 *   ch - source cell code
 *
 * out:
 *   pointer to first byte of source cell if code was valid
 */

UBYTE *char_addr(UWORD ch)
{
    UWORD offs;

    /* test against limits */
    if (ch >= v_fnt_st) {        /* v_fnt_st: ascii code of first cell in font */
        if (ch <= v_fnt_nd) {    /* v_fnt_nd: ascii code of last cell in font */
            /* getch offset from offset table */
            offs = v_off_ad[ch]; /* v_off_ad: pointer to current monospace font */
            offs >>= 3;          /* convert from pixels to bytes. */

            /* return valid address */
            return (UBYTE*)v_fnt_ad + offs;
        }
    }

    /* invalid code. no address returned */
    return NULL;
}