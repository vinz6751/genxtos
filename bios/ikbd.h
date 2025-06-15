/*
 * ikbd.h - Intelligent keyboard routines
 *
 * Copyright (C) 2001-2021 The EmuTOS development team
 *
 * Authors:
 *  LVL   Laurent Vogel
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#ifndef IKBD_H
#define IKBD_H

#include "biosdefs.h"

void ikbd_reset(void);

/* Mouse protocol (part of) */
#define MOUSE_REL_POS_REPORT    0xf8    /* values for mouse_packet[0] */
#define RIGHT_BUTTON_DOWN       0x01    /* these values are OR'ed in */
#define LEFT_BUTTON_DOWN        0x02

/* Baud rate used by Atari keyboards */
#define IKBD_BAUD 7812

/*
 * Date/Time to use when the hardware clock is not set.
 * We use the OS creation date at 00:00:00
 */
#define DEFAULT_DATETIME MAKE_ULONG(os_header.os_dosdate, 0)

LONG bcostat4(void);
LONG bconout4(WORD dev, WORD c);
void ikbdws(WORD cnt, const UBYTE *ptr);
void ikbd_writeb(UBYTE b);
void ikbd_writew(WORD w);

#if CONF_SERIAL_CONSOLE
void push_ascii_ikbdiorec(UBYTE ascii);
#endif

#if CONF_WITH_FLEXCAN || CONF_SERIAL_IKBD || defined(MACHINE_LISA)
#define call_ikbdraw(b) ikbdraw(b)
#endif

#endif /* IKBD_H */
