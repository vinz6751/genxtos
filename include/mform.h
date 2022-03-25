/*
 * mform.h - The mouse cursor format
 *
 * Copyright (C) 2019-2022 The EmuTOS development team
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#ifndef _MFORM_H
#define _MFORM_H

#include "portab.h"


/* There's a bit of a mess around this structure, which is why it has its own file:
 * The Line A needs to initialize the mouse cursor to some form, by default it's an arrow that is
 * modeled in MFORM format. For this reason, linea_mouse_set_form takes a MFORM. But MFORM is an AES
 * structure so it shouldn't be used in the Line A which is a lower layer...
 * Internally the Line A (for Atari) uses its own MCDB format so it doesn't even care about MFORM.
 * If we wanted the Line A to be standalone, we should then model the arrow in MCDB format, so we can
 * forget about MFORM in Line A.
 */

/* mouse form used by graf_mouse() etc */
typedef struct mform
{
    WORD    mf_xhot;
    WORD    mf_yhot;
    WORD    mf_nplanes;
    WORD    mf_bg;          /* mask colour index */
    WORD    mf_fg;          /* data colour index */
    UWORD   mf_mask[16];
    UWORD   mf_data[16];
} MFORM;


extern const MFORM arrow_mform;

#endif