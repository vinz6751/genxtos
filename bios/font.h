/*
 * font.h - font specific definitions
 *
 * Copyright (C) 2001 Lineo, Inc.
 * Copyright (C) 2004 by Authors (see below)
 * Copyright (C) 2015-2022 The EmuTOS development team
 *
 * Authors:
 *  MAD     Martin Doering
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#ifndef FONT_H
#define FONT_H

#include "fonthdr.h"
#include "portab.h"

/* prototypes */

void font_init(void);               /* initialize BIOS font ring */
Fonthead *font_set_default(void);   /* choose the default font */
UBYTE *char_addr(UWORD ch);         /* get address of a character number in font file */

#endif /* FONT_H */
