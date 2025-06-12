/*
 * ikbd.c - Atari IKBD keyboard controller routines
 *
 * Copyright (C) 2001-2022 The EmuTOS development team
 *
 * Authors:
 *  LVL   Laurent Vogel
 *  MAD   Martin Doering
 *        Robert de Vries (Linux m68k)
 *        Bjoern Brauel   (Linux m68k)
 *        Roman Hodek     (Linux m68k)
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 *
 * Some code I got from Linux m68k, thanks to the authors! (MAD)
 */

/*
 * not supported:
 * - alt-help screen hardcopy
 * - KEYTBL.TBL config with _AKP cookie (tos 5.00 and later)
 */

/*#define ENABLE_KDEBUG*/

#include "emutos.h"
#include "aciavecs.h"
#include "country.h"
#include "acia.h"
#include "tosvars.h"
#include "biosext.h"
#include "lineavars.h"
#include "iorec.h"
#include "asm.h"
#include "ikbd.h"
#include "sound.h"              /* for keyclick */
#include "scancode.h"
#include "delay.h"
#include "bios.h"
#include "coldfire.h"
#include "amiga.h"
#include "lisa.h"
#include "a2560u_bios.h"


/* forward declarations */

static UBYTE ikbd_readb(WORD timeout);


/*
 * reset the IKBD
 *
 * According to Atari documentation, the IKBD answers to the Reset command
 * with a version byte when the reset is complete.  At one time we just
 * waited for a byte before sending further commands.  However, if we are
 * rebooting because of Ctrl+Alt+Del, and we keep Ctrl+Alt+Del held down,
 * a real IKBD transmits the following within about 100msec of the reset
 * command (info courtesy Christian Zietz):
 *  0x9d    Ctrl released
 *  0xf1    version byte
 *  0x1d    Ctrl pressed
 *  0x38    Alt pressed
 *  0x53    Del pressed
 * As a result, EmuTOS sees another reboot request, and will reboot
 * continually while the keys are held down.  Atari TOS does not have this
 * problem, because it just delays for approx 312msec after the reset and
 * does not see the additional keys.
 *
 * It is difficult to do exactly the same as Atari TOS, because the only
 * timing available on all systems at this point is the loop counter, and
 * this is not yet calibrated.  If the delay is too short, we may see the
 * spurious bytes; if the delay is too long, it not only slows down the
 * boot process, it may also cause problems with IKBD replacement hardware.
 * For example, the QWERTYX hardware will not respond if there is too great
 * a delay between the reset and data from the OS (the 'Set date' command).
 *
 * The following code is supposed to handle all this ...
 */
void ikbd_reset(void)
{
    UBYTE version;

    ikbd_writew(0x8001);            /* reset */

    /* first, wait for the version byte */
    while(1)
    {
        version = ikbd_readb(300);  /* 'standard' 300msec timeout */
        if (version == 0)           /* timeout, we give up */
            return;

        if ((version&0xf0) == 0xf0) /* version byte, usually 0xf1 */
        {
            KDEBUG(("ikbd_version = 0x%02x\n", version));
            break;
        }
    }

    /* eat any pending keyboard bytes */
    while(ikbd_readb(5))    /* timeout long enough for pending data */
        ;
}


/* send a byte to the IKBD - for general use */
void ikbd_writeb(UBYTE b)
{
    KDEBUG(("ikbd_writeb(0x%02x)\n", (UBYTE)b));

    while (!bcostat4());
#if CONF_WITH_IKBD_ACIA
    ikbd_acia.data = b;
#elif CONF_WITH_FLEXCAN
    coldfire_flexcan_ikbd_writeb(b);
#elif defined(MACHINE_AMIGA)
    amiga_ikbd_writeb(b);
#endif
}

/* send a word to the IKBD as two bytes - for general use */
void ikbd_writew(WORD w)
{
    ikbd_writeb(HIBYTE(w));
    ikbd_writeb(LOBYTE(w));
}

/* Read a byte from the IKBD, with a timeout in msec.
 * This must not be used when interrupts are enabled.
 */
static UBYTE ikbd_readb(WORD timeout)
{
#if CONF_WITH_IKBD_ACIA
    WORD i;

    /* We have to use a timeout to avoid waiting forever
     * if the keyboard is unplugged.
     */
    for (i = 0; i < timeout; i++)
    {
        if (ikbd_acia.ctrl & ACIA_RDRF)
            return ikbd_acia.data;

        delay_loop(loopcount_1_msec);
    }

    return 0; /* bogus value when timeout */
#else
    return 0; /* bogus value */
#endif
}


#if CONF_SERIAL_CONSOLE

/* Find a scancode in a keyboard table */
static UBYTE scancode_from_ascii(UBYTE ascii, const UBYTE *table)
{
    int i;

    for (i = 0; i < 128; i++)
    {
        if (table[i] == ascii)
            return i;
    }

    return 0;
}

/* Guess full KBD record from ASCII character */
static ULONG ikbdiorec_from_ascii(UBYTE ascii)
{
    UBYTE scancode = 0;
    UBYTE mode = 0;
    ULONG value;

    /* We have to look in the keyboard tables to find a matching scancode.
     * First, look in Normal table. */
    scancode = scancode_from_ascii(ascii, current_keytbl.norm);

    /* If not found, look in Shift table. */
    if (scancode == 0)
    {
        scancode = scancode_from_ascii(ascii, current_keytbl.shft);
        if (scancode != 0)
            mode = MODE_LSHIFT;
    }

    /* Then check for Control codes from ^A to ^Z. */
    if (scancode == 0 && (ascii >= ('a'-'`') && ascii <= ('z'-'`')))
    {
        scancode = scancode_from_ascii('`'+ascii, current_keytbl.norm);
        if (scancode != 0)
            mode = MODE_CTRL;
    }

    value = MAKE_ULONG(scancode, ascii);
    value |= (ULONG)mode << 24;

    return value;
}

/* Emulate a key press from an ASCII character */
void push_ascii_ikbdiorec(UBYTE ascii)
{
    ULONG value;

    value = ikbdiorec_from_ascii(ascii);
    iorec_put_long(&ikbdiorec, value);
}

#endif /* CONF_SERIAL_CONSOLE */



/*=== kbrate (xbios) =====================================================*/



/*=== ikbd acia stuff ==================================================*/


