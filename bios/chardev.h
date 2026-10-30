/*
 * chardev.h - bios devices
 *
 * Copyright (C) 2001-2019 The EmuTOS development team
 *
 * Authors:
 *  MAD   Martin Doering
 *  LVL   Laurent Vogel
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#ifndef _CHARDEV_H
#define _CHARDEV_H

/*
 * These are installed in bcon*_vec[] and invoked via protect_w/protect_ww,
 * which always pass arguments on the stack.  Under FASTCALL=1 they must be
 * cdecl or trap #13 Bconout/Bconin see garbage (invisible console text).
 */
LONG ARGS_ON_STACK bconout2(WORD, WORD);
LONG ARGS_ON_STACK bconout5(WORD, WORD);
LONG ARGS_ON_STACK bcostat2(void);

LONG ARGS_ON_STACK char_dummy(void);
LONG ARGS_ON_STACK charout_dummy(WORD dev, WORD x);


/* internal init routine */

void chardev_init(void);

#endif /* _CHARDEV_H */
