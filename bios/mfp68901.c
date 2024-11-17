/* Definitions for the MFP68901 chip
 * Copyright (C) 2001 Martin Doering
 * Copyright (C) 2001-2022 The EmuTOS development team
 *
 * Authors:
 *  MAD     Martin Doering
 *  LVL     Laurent Vogel
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#include "config.h"

#if CONF_WITH_MFP || CONF_WITH_TT_MFP

#include "mfp68901.h"


void mfp68901_reset_regs(MFP *mfp)
{
    volatile UBYTE *p;
    /*
     * The following writes zeroes to everything except the UDR (anything
     * written to the UDR would be sent as soon as the baud rate clock
     * (Timer D) was enabled).  We avoid writing to the even addresses,
     * because some buggy emulators (I'm looking at you, STonXDOS)
     * generate bus errors there.
     */
    for (p = &mfp->gpip; p <= &mfp->tsr; p += 2)
        *p = 0;
}



void mfp68901_disable_interrupt(MFP *mfp, WORD num)
{
    UBYTE mask;

    num &= 0x0F;
    if (num >= 8) {
        mask = ~(1<<(num-8));
        mfp->imra &= mask;
        mfp->iera &= mask;
        mfp->ipra = mask;   /* note: IPRA/ISRA ignore '1' bits */
        mfp->isra = mask;
    }
    else {
        mask = ~(1<<num);
        mfp->imrb &= mask;
        mfp->ierb &= mask;
        mfp->iprb = mask;   /* note: IPRB/ISRB ignore '1' bits */
        mfp->isrb = mask;
    }
}


void mfp68901_enable_interrupt(MFP *mfp, WORD num)
{
    UBYTE mask;

    num &= 0x0F;
    if (num >= 8) {
        mask = 1 << (num - 8);
        mfp->iera |= mask;
        mfp->imra |= mask;
    } else {
        mask = 1 << num;
        mfp->ierb |= mask;
        mfp->imrb |= mask;
    }
}

#endif
