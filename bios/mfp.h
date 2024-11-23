/*
 *  mfp.h - BIOS/XBIOS functions related to the 68901 MFP implemented in the Atari
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

#include "config.h"

#include "mfp68901.h"


#if CONF_WITH_TT_MFP

#define TT_MFP_BASE     ((MFP *)(0xfffffa80L))

void mfptt_init(void);
void tt_mfpint(WORD num, LONG vector);

#endif


#if CONF_WITH_MFP

/*==== Defines ============================================================*/
#define MFP_BASE        ((MFP *)(0xfffffa00L))
#define MFP_GPIP        0xfffffa01L

/*==== Xbios functions ====================================================*/

void mfpint(WORD num, LONG vector);
void jdisint(WORD num);
void jenabint(WORD num);

/*==== internal functions =================================================*/

void mfp_init(void);
void mfp_setup_timer(MFP *mfp,WORD timer, WORD control, WORD data);




# if CONF_WITH_FDC || CONF_WITH_ACSI

/* Returns TRUE if the timeout elapsed before the gpip changed. The delay is in 200Hz ticks */
BOOL mfp_wait_disk_irq_with_timeout(LONG delay);

# endif

#endif /* CONF_WITH_MFP */

#endif /* MFP_H */
