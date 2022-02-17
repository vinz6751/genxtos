/*
 * linea_mouse_atari.c - ATARI-specific mouse support
 *
 * Copyright 1982 by Digital Research Inc.  All rights reserved.
 * Copyright 1999 by Caldera, Inc. and Authors:
 * Copyright 2002-2022 by The EmuTOS development team
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#include "emutos.h"

#ifndef MACHINE_A2560U

#include "linea.h"
#include "lineavars.h"


static MCS  *mcs_ptr;  /* ptr to Mouse Cursor Save area in use */
 

static void mouse_set_visible(WORD x, WORD y)
{
    linea_sprite_show_atari(&mouse_cdb, mcs_ptr, x, y);
}


static void mouse_set_invisible(void)
{
    linea_sprite_hide_atari(mcs_ptr);
}


static void mouse_move_to(WORD x, WORD y)
{
    linea_sprite_hide_atari(mcs_ptr);  /* remove the old cursor from the screen */
    linea_sprite_show_atari(&mouse_cdb, mcs_ptr, x, y); /* display the cursor */    
}


static void set_mouse_cursor(const MFORM *src) {
    int i;
    UWORD * gmdt;                /* global mouse definition table */
    MCDB *dst = &mouse_cdb;
    const UWORD * mask;
    const UWORD * data;
    
    /* save x-offset of mouse hot spot */
    dst->xhot = src->mf_xhot & 0x000f;

    /* save y-offset of mouse hot spot */
    dst->yhot = src->mf_yhot & 0x000f;

    /*
     * Move the new mouse definition into the global mouse cursor definition
     * table.  The values for the mouse mask and data are supplied as two
     * separate 16-word entities.  They must be stored as a single array
     * starting with the first word of the mask followed by the first word
     * of the data and so on.
     */

    /* copy the data to the global mouse definition table */
    gmdt = dst->maskdata;
    mask = src->mf_mask;
    data = src->mf_data;
    for (i = 15; i >= 0; i--) {
        *gmdt++ = *mask++;              /* get next word of mask */
        *gmdt++ = *data++;              /* get next word of data */
    }    
}


static void resolution_changed(void)
{
# if EXTENDED_PALETTE
    mcs_ptr = (v_planes <= 4) ? &mouse_cursor_save : &ext_mouse_cursor_save;
# else
    mcs_ptr = &mouse_cursor_save;
# endif    
}


const LINEA_MOUSE_RENDERER mouse_display_driver = {
    mouse_set_visible,
    mouse_set_invisible,
    mouse_move_to,
    set_mouse_cursor,
    resolution_changed
};

#endif