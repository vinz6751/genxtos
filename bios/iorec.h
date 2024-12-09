/*
 * iorec.h - Input Output RECords related things
 *
 * Copyright (C) 2001-2021 The EmuTOS development team
 *
 * Authors:
 *  LVL   Laurent Vogel
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#ifndef IOREC_H
#define IOREC_H

#include "emutos.h"

/*==== Structs ============================================================*/

typedef struct iorec IOREC;

/* IOREC is a circular buffer used both to store incoming data (e.g. from the
 * MIDI, serial port etc.) or data to send (serial port)
 */
struct iorec {
    UBYTE *buf;         /* input buffer */
    WORD size;          /* buffer size */
    volatile WORD head; /* head index */
    volatile WORD tail; /* tail index */
    WORD low;           /* low water mark */
    WORD high;          /* high water mark */
};

extern IOREC ikbdiorec, midiiorec;

/*==== Functions ==========================================================*/

UBYTE iorec_get(IOREC *iorec);
WORD  iorec_can_read(IOREC *iorec);
void  iorec_put_long(IOREC *iorec, ULONG value);
LONG  iorec_get_long(IOREC *iorec);

#endif /* IOREC_H */
