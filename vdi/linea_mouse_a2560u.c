/*
 * linea_mouse_a2560u.c - A2560U Foenix-specific mouse support
 *
 * Copyright 1982 by Digital Research Inc.  All rights reserved.
 * Copyright 1999 by Caldera, Inc. and Authors:
 * Copyright 2002-2022 by The EmuTOS development team
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#include "emutos.h"
#include "asm.h"

#ifdef MACHINE_A2560U

#include "linea.h"
#include "a2560u.h"


static void paint_mouse(MCDB *sprite, WORD x, WORD y)
{
    vicky2_show_mouse();
}


static void unpaint_mouse(void)
{
    vicky2_hide_mouse();
}


/* This is here rather than in a2560u.c because MFORM would required to include aesdefs.h there.
 * and I'd like the a2560u.c to not have dependencies on EmuTOS so it can be used elsewhere. */
static void set_mouse_cursor(const MFORM *src)
{
    /* No hot-spot support in the Foenix :( */

    int r,c;
    UWORD mask;
    UWORD data;
    UWORD *v = (UWORD*)VICKY_MOUSE_MEM;

    for (r = 0; r < 16; r++)
    {
        mask = src->mf_mask[r];
        data = src->mf_data[r];

        for (c = 0; c < 16; c++)
        {
            if (mask & 0x8000)
                v[c] = (data & 0x8000) 
                    ? src->mf_fg
                    : src->mf_bg;
            else
                v[c] = 0;
            mask <<= 1;
            data <<= 1;
        }
    }
}


const LINEA_MOUSE_RENDERER mouse_display_driver = {
    just_rts, /* init() */
    just_rts, /* deinit() */
    just_rts, /* draw */
    paint_mouse,
    unpaint_mouse,
    set_mouse_cursor,
    just_rts /* resolution_changed */
};

#endif
