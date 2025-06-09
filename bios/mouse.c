/*
 * mouse.c - Mouse vector setting for XBIOS 0
 *
 * Copyright (C) 2001-2019 The EmuTOS development team
 * Copyright (C) 1995-1998 Russell King <linux@arm.linux.org.uk>
 *
 * Authors:
 *  LVL   Laurent Vogel
 *  MAD   Martin Doering
 *        Russell King <linux@arm.linux.org.uk>
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 *
 * Some code I got from Linux m68k, thanks to the authors! (MAD)
 */

/* #define ENABLE_KDEBUG */

#include "emutos.h"
#include "asm.h"
#include "bios.h"
#include "lineavars.h"
#include "ikbd.h"
#include "mouse.h"
#include "tosvars.h"
#include "vectors.h"


/*
 * mouse initialization. Programs the IKBD to report mouse events
 * (or disable), and set handling vector for IKBD mouse packets.
 * new_mousevec is a handler for the IKBD mousevec vectors, it receives the IKBD packet in a0
 */

void Initmous(WORD mouse_mode, struct initmous_parameter_block *p, void(*new_mousevec)(UBYTE*))
{
#if defined(MACHINE_A2560U) || defined(MACHINE_A2560X)
    // These machines have no IKBD
    kbdvecs.mousevec = (new_mousevec != NULL) ? new_mousevec : just_rts;
#else
    BOOL error = 0;

    switch (mouse_mode) {

    case DISABLE_MOUSE:
        ikbd_writeb(0x12);      /* disable mouse */
        break;

    case RELATIVE_MOUSE:
        /* Parameters for relative mouse movement */
        if (p != NULL) {
            ikbd_writeb(0x08);          /* set relative mouse mode */

            ikbd_writeb(0x0b);          /* set relative threshold */
            ikbd_writeb(p->xparam);
            ikbd_writeb(p->yparam);
        }
        break;

    case ABSOLUTE_MOUSE:
        /* Parameters for absolute mouse movement */
        if (p != NULL) {
            ikbd_writeb(0x09);          /* set absolute position */
            ikbd_writew(p->xmax);
            ikbd_writew(p->ymax);

            ikbd_writeb(0x0c);          /* set mouse scale */
            ikbd_writeb(p->xparam);
            ikbd_writeb(p->yparam);

            ikbd_writeb(0x0e);          /* set initial position */
            ikbd_writeb(0x00);          /* dummy */
            ikbd_writew(p->xinitial);
            ikbd_writew(p->yinitial);
        }
        break;

    case KEYCODE_MOUSE:
        /*
         * mouse_kbd_mode - Set mouse keycode mode
         *
         * Set mouse monitoring routines to return cursor motion keycodes instead of
         * either RELATIVE or ABSOLUTE motion records. The ikbd returns the
         * appropriate cursor keycode after mouse travel exceeding the user specified
         * deltas in either axis. When the keyboard is in key scan code mode, mouse
         * motion will cause the make code immediately followed by the break code.
         * Note that this command is not affected by the mouse motion origin.
         *
         * deltax - distance in X clicks to return (LEFT) or (RIGHT)
         * deltay - distance in Y clicks to return (UP) or (DOWN)
         */

        if (p != NULL) {
            ikbd_writeb(0x0a);          /* set keyboard mode */
            ikbd_writeb(p->xparam);
            ikbd_writeb(p->yparam);
        }
        break;
    default:
        error = TRUE;
    }

    if (error || mouse_mode == DISABLE_MOUSE) {
        kbdvecs.mousevec = (void(*)(UBYTE*))just_rts;    /* set dummy vector */
        return;
    }


    /* we're setting the mouse to some mode, so we need to passe in some parameters */
    if (p != NULL) {
        if (p->topmode == IN_YBOT)
            ikbd_writeb(0x0f);      /* set bottom to y=0 */
        if (p->topmode == IN_YTOP)
            ikbd_writeb(0x10);      /* set top to y=0 */

        ikbd_writeb(0x07);          /* set mouse button reaction */
        ikbd_writeb(p->buttons);
    }
    if (new_mousevec != NULL)
        kbdvecs.mousevec = new_mousevec;  /* set mouse vector */
#endif
}
