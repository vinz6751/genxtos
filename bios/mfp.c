/*
 * mfp.c - handling of the Multi-Function Peripheral MFP 68901
 *
 * Copyright (C) 2001 Martin Doering
 * Copyright (C) 2001-2022 The EmuTOS development team
 *
 * Authors:
 *  LVL   Laurent Vogel
 *  MAD   Martin Doering
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#include "emutos.h"
#include "string.h"
#include "mfp.h"
#include "tosvars.h"
#include "vectors.h"
#include "coldfire.h"
#include "lisa.h"
#include "a2560u_bios.h"

#if CONF_WITH_MFP || CONF_WITH_TT_MFP
# include "mfp68901.h"
#endif


#if CONF_WITH_TT_MFP

void mfptt_init(void)
{
    MFP *mfp = TT_MFP_BASE; /* set base address of MFP */

    mfp68901_reset_regs(mfp);    /* reset the MFP registers */
    mfp->vr = 0x58;         /* vectors 0x50 to 0x5F, software end of interrupt */
}

void tt_mfpint(WORD num, LONG vector)
{
    num &= 0x0F;
    mfp68901_disable_interrupt(TT_MFP_BASE, num);
    *(LONG *)((0x50L + num)*4) = vector;
    mfp68901_enable_interrupt(TT_MFP_BASE, num);
}

#endif


#if CONF_WITH_MFP

/*==== mfp_init - initialize the MFP ========================================*/

void mfp_init(void)
{
    MFP *mfp = MFP_BASE;    /* set base address of MFP */

    mfp68901_reset_regs(mfp);    /* reset the MFP registers */
    mfp->vr = 0x48;         /* vectors 0x40 to 0x4F, software end of interrupt */
}


/*==== xbios functions ===========================================*/

void mfpint(WORD num, LONG vector)
{
    num &= 0x0F;
    jdisint(num);
    *(LONG *)((0x40L + num)*4) = vector;
    jenabint(num);
}


void jdisint(WORD num)
{
    mfp68901_disable_interrupt(MFP_BASE, num);
}


void jenabint(WORD num)
{
    mfp68901_enable_interrupt(MFP_BASE, num);
}


/* setup the timer, but do not activate the interrupt */
void mfp_setup_timer(MFP *mfp, WORD timer, WORD control, WORD data)
{
    switch(timer) {
    case 0:  /* timer A */
        mfp->tacr = 0;
        mfp->tadr = data;
        mfp->tacr = control;
        break;
    case 1:  /* timer B */
        mfp->tbcr = 0;
        mfp->tbdr = data;
        mfp->tbcr = control;
        break;
    case 2:  /* timer C */
        mfp->tcdcr &= 0x0F;
        mfp->tcdr = data;
        mfp->tcdcr |= control & 0xF0;
        break;
    case 3:  /* timer D */
        mfp->tcdcr &= 0xF0;
        mfp->tddr = data;
        mfp->tcdcr |= control & 0x0F;
        break;
    default:
        return;
    }
}


/* returns 1 if the timeout (in clock ticks) elapsed before gpip went low */
int mfp_wait_fdc_hdc_irq_with_timeout(LONG delay)
{
    MFP *mfp = MFP_BASE;
    LONG next = hz_200 + delay;

    while(hz_200 < next) {
        if((mfp->gpip & 0x20) == 0) {
            return 0;
        }
    }
    return 1;
}

#endif /* CONF_WITH_MFP */



