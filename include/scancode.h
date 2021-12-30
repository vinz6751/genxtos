/*
 * scancode.h - invariant IKBD scancode definitions
 *
 * This file exists to centralise the definition of invariant IKBD
 * scancodes: those scancodes which do not change, regardless of
 * language.  These were previously defined in many places.
 *
 * Copyright (C) 2016-2019 The EmuTOS development team
 *
 * Authors:
 *  RFB    Roger Burrows
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#ifndef _SCANCODE_H
#define _SCANCODE_H

#include "config.h"

/* Raw scancodes */
#ifdef MACHINE_A2560U
# define KEY_UPARROW 0x67
# define KEY_LTARROW 0x69
# define KEY_RTARROW 0x6a
# define KEY_DNARROW 0x6c
# define KEY_DELETE  0x6f
# define KEY_INSERT  0x6e
# define KEY_HOME    0x66
/* PS/2 specific */
# define KEY_RCTRL   0x61
# define KEY_ALTGR   0x64
# define KEY_PAGEUP  0x68
#else
# define KEY_HELP    0x62
# define KEY_UPARROW 0x48
# define KEY_LTARROW 0x4b
# define KEY_RTARROW 0x4d
# define KEY_DNARROW 0x50
# define KEY_DELETE  0x53
# define KEY_INSERT  0X52
# define KEY_HOME    0X47
#endif


/* scancode definitions */
#define KEY_RELEASED 0x80       /* This bit set, when key-release scancode */

#define KEY_LSHIFT  0x2a        /* modifiers */
#define KEY_RSHIFT  0x36
#define KEY_CTRL    0x1d
#define KEY_ALT     0x38
#define KEY_CAPS    0x3a

#define KEY_ESCAPE    0x01        /* invariant keys, unaffected by modifiers */
#define KEY_BACKSPACE 0x0e
#define KEY_TAB       0x0f
#ifndef MACHINE_A2560U
# define KEY_UNDO     0x61
#endif

#define KEY_RETURN  0x1c        /* semi-invariant, ctrl changes ascii to newline */
#define KEY_ENTER   0x72

#define KEY_F1      0x3b        /* function keys F1 - F10 */
#define KEY_F10     0x44

/* IKBD scan codes (0xssaa ss: scancode, aa: ascii) */
#define IKBD_SCANCODE(a,b) (((a) << 8) + (b))
#define ESCAPE              IKBD_SCANCODE(KEY_ESCAPE,0x1b)
#define BACKSPACE           IKBD_SCANCODE(KEY_BACKSPACE, 0x08)
#define TAB                 IKBD_SCANCODE(KEY_TAB, 0x09)
#define RETURN              IKBD_SCANCODE(KEY_RETURN, 0x0d)
#define DELETE              IKBD_SCANCODE(KEY_DELETE, 0x7f)
# define ENTER              IKBD_SCANCODE(KEY_ENTER, 0x0d)

#ifdef MACHINE_A2560U
#else
# define UNDO                0x6100
#endif

/*
 * arrow keys
 */
#define ARROW_UP            IKBD_SCANCODE(KEY_UPARROW, 0)
#define ARROW_DOWN          IKBD_SCANCODE(KEY_DNARROW, 0)
#define ARROW_LEFT          IKBD_SCANCODE(KEY_LTARROW, 0)
#define ARROW_RIGHT         IKBD_SCANCODE(KEY_RTARROW, 0)

#define SHIFT_ARROW_UP      IKBD_SCANCODE(KEY_UPARROW, 0x38)
#define SHIFT_ARROW_DOWN    IKBD_SCANCODE(KEY_DNARROW, 0x32)
#define SHIFT_ARROW_LEFT    IKBD_SCANCODE(KEY_LTARROW, 0x34)
#define SHIFT_ARROW_RIGHT   IKBD_SCANCODE(KEY_RTARROW, 0x36)

#define CTRL_ARROW_LEFT     0x7300
#define CTRL_ARROW_RIGHT    0x7400

/*
 * function keys
 */
#define FUNKEY_01           IKBD_SCANCODE(KEY_F1, 0)
#define FUNKEY_10           IKBD_SCANCODE(KEY_F10, 0)
#define FUNKEY_11           0x5400
#define FUNKEY_20           0x5d00

#endif /* _SCANCODE_H */
