/* iorec.c - Functions for manipulating ring buffers
 *
 * Copyright (C) 2001-2019 Martin Doering
 *
 * Authors:
 *  MAD   Martin Doering
 *  LVL   Laurent Vogel
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#include "emutos.h"
#include "asm.h"
#include "iorec.h"


UBYTE iorec_get(IOREC *iorec) {
    WORD old_sr;
    LONG value;

    /* disable interrupts */
    old_sr = set_sr(0x2700);

    iorec->head++;
    if (iorec->head >= iorec->size)
        iorec->head = 0;

    value = *(UBYTE *)(iorec->buf + iorec->head);

    /* restore interrupts */
    set_sr(old_sr);

    return value;
}


LONG  iorec_get_long(IOREC *iorec) {
    WORD old_sr;
    LONG value;

    /* disable interrupts */
    old_sr = set_sr(0x2700);

    iorec->head += 4;
    if (iorec->head >= iorec->size) {
        iorec->head = 0;
    }
    value = *(ULONG_ALIAS *) (iorec->buf + iorec->head);

    /* restore interrupts */
    set_sr(old_sr);

    return value;
}


void iorec_put_long(IOREC *iorec, ULONG value) {
    short tail;

    KDEBUG(("KBD iorec: Pushing value 0x%08lx\n", value));

    tail = iorec->tail + 4;
    if (tail >= iorec->size) {
        tail = 0;
    }
    if (tail == iorec->head) {
        /* iorec full */
        return;
    }
    *(ULONG_ALIAS *) (iorec->buf + tail) = value;
    iorec->tail = tail;
}


WORD iorec_can_read(IOREC *iorec) {
    return iorec->head == iorec->tail ? 0 : -1;
}
