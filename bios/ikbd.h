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

/*
 * These bit flags are set in the "shifty" byte based on the state
 * of control keys on the keyboard, like:
 * right shift, left shift, control, alt, ...
 * Public flags are defined in include/biosdefs.h and returned by Kbshift().
 * Private BIOS combinations are defined below.
 */

#define MODE_SHIFT  (MODE_RSHIFT|MODE_LSHIFT)   /* shifted */

#define HOTSWITCH_MODE (MODE_LSHIFT|MODE_ALT)

/* Mouse protocol (part of) */
#define MOUSE_REL_POS_REPORT    0xf8    /* values for mouse_packet[0] */
#define RIGHT_BUTTON_DOWN       0x01    /* these values are OR'ed in */
#define LEFT_BUTTON_DOWN        0x02

/*
 * dead key support: i.e. hitting ^ then a yields â.
 * char codes DEADMIN to DEADMAX inclusive are reserved for dead keys.
 * table keytbl->dead[i - DEADMIN], DEADMIN <= i <= DEADMAX, gives the
 * list of couples of (before, after) char codes ended by zero.
 */

/* We use range 1 to 7, as it is best to keep 0 for unallocated keys?
 * and 8 is taken by backspace.
 */
#define DEADMIN 1
#define DEADMAX 7
#define DEAD(i) (i + DEADMIN)

/*
 * bit flags in 'features' in struct keytbl:
 *  DUAL_KEYBOARD indicates that:
 *      a) the "altXXXX" pointers point to arrays of full scancode->ascii
 *         mappings
 *      b) HOTSWITCH_MODE in shifty will toggle between the "XXXX" and
 *         "altXXXX" arrays for scancode decoding
 *
 */
#define DUAL_KEYBOARD   0x0001


/* Baud rate used by Atari keyboards */
#define IKBD_BAUD 7812

/*
 * Date/Time to use when the hardware clock is not set.
 * We use the OS creation date at 00:00:00
 */
#define DEFAULT_DATETIME MAKE_ULONG(os_header.os_dosdate, 0)

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
