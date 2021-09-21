/*
 *  biosmem.h - dumb bios-level memory management
 *
 * Copyright (C) 2002-2020 The EmuTOS development team
 *
 * Authors:
 *  LVL    Laurent Vogel
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#ifndef BIOSMEM_H
#define BIOSMEM_H

#include "bdosdefs.h"
#include "biosdefs.h"
#include "biosext.h"

extern UBYTE dskbuf[DSKBUF_SIZE]; /* In ST-RAM */

/* Prototypes */
void bmem_init(void);

/* BIOS functions */

void getmpb(MEMORY_PARTITION_BLOCK *mpb);
UBYTE *balloc_stram(ULONG size, BOOL top);

#endif /* BIOSMEM_H */
