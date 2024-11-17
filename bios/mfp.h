/*
 *  mfp.h - BIOS/XBIOS functions related to the 68901 MFP 
 *
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

#ifndef MFP_H
#define MFP_H

#include "mfp68901.h"

#if CONF_WITH_TT_MFP

#define TT_MFP_BASE     ((MFP *)(0xfffffa80L))

void mfptt_init(void);
void tt_mfpint(WORD num, LONG vector);

#endif


#if CONF_WITH_MFP

/*==== Defines ============================================================*/
#define MFP_BASE        ((MFP *)(0xfffffa00L))

/*==== Xbios functions ====================================================*/

void mfpint(WORD num, LONG vector);
void jdisint(WORD num);
void jenabint(WORD num);

/*==== internal functions =================================================*/

void mfp_init(void);
void mfp_setup_timer(MFP *mfp,WORD timer, WORD control, WORD data);

/* function which returns 1 if the timeout elapsed before the gpip changed */
int mfp_wait_fdc_hdc_irq_with_timeout(LONG delay);   /* delay in ticks */

/*==== Xbios functions ====================================================*/

void xbtimer(WORD timer, WORD control, WORD data, LONG vector);

#endif /* CONF_WITH_MFP */

/*==== internal functions =================================================*/

void init_system_timer(void);

/* "sieve" to get only the fourth interrupt, 0x1111 initially */
extern WORD timer_c_sieve;

#endif /* MFP_H */
