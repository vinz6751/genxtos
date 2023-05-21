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
#include "a2560u_bios.h"

static void mouse_set_visible(WORD x, WORD y)
{
    // TODO: fake a PS/2 packet here to set coordinates
    a2560u_debugnl("mouse_set_visible @ %d,%d", x, y);
    vicky_mouse_show();
}


static void mouse_set_invisible(void)
{
    a2560u_debugnl("mouse_set_invisible");
    vicky_mouse_hide();
}


/* This is here rather than in a2560u.c because MFORM would required to include aesdefs.h there.
 * and I'd like the a2560u.c to not have dependencies on EmuTOS so it can be used elsewhere. */
static void set_mouse_cursor(const MFORM *src)
{
    /* No hot-spot support in the Foenix :( */
    int r,c; /* row, column */
    UWORD mask;
    UWORD data;
    UWORD *v = (UWORD*)VICKY_MOUSE_MEM;
    
    for (r = 0; r < 16; r++)
    {
        mask = src->mf_mask[r];
        data = src->mf_data[r];

        //a2560u_debugnl("%04x %04x", mask, data);
        for (c = 0; c < 16; c++)
        {
            if (mask & 0x8000)
            {                
                // 2 words, GB AR then GB
                if (data & 0x8000)
                {
                    //a2560u_debug("X");
                    /* Black */
                    *v++= 0x0000;
                    *v++= 0xff00;
                }
                else
                {
                    //a2560u_debug("O");
                    /* White */
                    *v++ = 0xffff;
                    *v++ = 0xffff;
                }
            }
            else
            {
                //a2560u_debug("-");
                /* Transparent */
                *v++ = 0;
                *v++ = 0;
            }
            mask <<= 1;
            data <<= 1;
        }
        //a2560u_debugnl("");
    }
}


const LINEA_MOUSE_RENDERER mouse_display_driver = {
    mouse_set_visible,
    mouse_set_invisible,
    just_rts,
    set_mouse_cursor,
    just_rts /* resolution_changed */
};

#endif
