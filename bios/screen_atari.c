/*
 * screen_atari.c - Atari ST & derivatives-specific low-level screen routines
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
#include "asm.h"
#include "dmasound.h"
#include "has.h"
#include "mfp.h"
#include "screen.h"
#include "screen_atari.h"
#include "screen_tt.h"
#include "tosvars.h"

#if CONF_WITH_ATARI_VIDEO

/*
 * Initialise ST(e) palette registers
 */
void initialise_ste_palette(UWORD mask)
{
    volatile UWORD *col_regs = (UWORD *) ST_PALETTE_REGS;
    int i;

    for (i = 0; i < 16; i++)
        col_regs[i] = default_palette[i] & mask;
}


/*
 * Fixup ST(e) palette registers so the highest available color is
 * always "black" (by default)
 */
void fixup_ste_palette(WORD rez)
{
    volatile UWORD *col_regs = (UWORD *) ST_PALETTE_REGS;

    if (rez == ST_MEDIUM)
        col_regs[3] = col_regs[15];
    else if (rez == ST_HIGH)
        col_regs[1] = col_regs[15];
}


/*
 * detect_monitor_change(): called by VBL interrupt handler
 *
 * this checks if the current monitor mode (monochrome/colour) is the
 * same as that set in the shifter.  if not, it calls swv_vec() which
 * by default does a system restart.
 */
void detect_monitor_change(void)
{
    SBYTE monoflag;
    volatile SBYTE *gpip = ((volatile SBYTE *)0xfffffa01L);
    volatile UBYTE *shifter;
    UBYTE monores;
    UBYTE curres;
    UBYTE newres;

    /* not supported on VIDEL */
    if (HAS_VIDEL)
        return;

    monoflag = *gpip;
#if CONF_WITH_DMASOUND
    if (HAS_DMASOUND)
    {
        WORD sr = set_sr(0x2700);
        SBYTE monoflag2;
        SBYTE dmaplay;

        /*
         * on systems with DMA sound, the 'DMA sound active' bit (bit 0
         * of 0xffff8901) is XOR'ed with the monochrome detect bit before
         * being presented at MFP GPIP bit 7.  therefore we must read both
         * bits in order to determine the monitor type.  since the 'sound
         * active' bit can be changed by the hardware at any time, we must
         * avoid a race condition.  the following code waits for both the
         * 'sound active' bit and MFP GPIP bit 7 to stabilise before
         * determining the type of monitor connected.
         */
        for (;;)
        {
            dmaplay = *((volatile SBYTE *)DMASOUND_CTRL);
            monoflag = *gpip;
            monoflag2 = *gpip;
            if ((monoflag ^ monoflag2) < 0)
                continue;
            if (*((volatile SBYTE *)DMASOUND_CTRL) == dmaplay)
                break;
        }

        set_sr(sr);
        if (dmaplay & 1)
            monoflag = -monoflag;
    }
#endif

#if CONF_WITH_TT_SHIFTER
    if (HAS_TT_SHIFTER)
    {
        shifter = ((volatile UBYTE *)TT_SHIFTER);
        curres = *shifter & 7;
        monores = TT_HIGH;
    }
    else
#endif
    /* Assume ST(e) shifter */
    {
        shifter = ((volatile UBYTE *)ST_SHIFTER);
        curres = *shifter & 3;
        monores = ST_HIGH;
    }

    if (curres == monores)  /* current resolution is mono */
    {
        if (monoflag >= 0)  /* mono monitor detected */
            return;
        /* colour monitor detected: switch resolution */
        newres = defshiftmod;   /* use default shifter mode */
        if (newres == monores)  /* but if it's mono, make it ST LOW */
            newres = ST_LOW;
    }
    else        /* current resolution is a colour resolution */
    {
        if (monoflag < 0)   /* & colour monitor detected */
            return;
        /* mono monitor detected: switch resolution */
#if 0
        /*
         * TOS 2.06 & 3.06 (at least) call this here to wait until just
         * after a VBL.  it is surmised that this is because:
         * (a) experience shows that at least some video hardware
         *     misbehaves if the shifter value is not changed 'soon'
         *     after the interrupt, and
         * (b) in TOS 2/3, the vblqueue is processed before this routine
         *     is called, and thus lengthy vblqueue function(s) could
         *     trigger the misbehaviour.
         */
        vsync();
#endif
        newres = monores;
    }

    sshiftmod = newres;
    *shifter = (*shifter & 0xf8) | newres;
    (*swv_vec)();
}

#endif
