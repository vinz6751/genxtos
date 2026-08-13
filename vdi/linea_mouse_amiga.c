/*
 * linea_mouse_amiga.c - software mouse rendering for Amiga planar screens
 *
 * Copyright (C) 2026 The EmuTOS development team
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#include "emutos.h"

#if defined(MACHINE_AMIGA) && CONF_WITH_SOFTWARE_MOUSE_RENDERING

#include "linea.h"
#include "lineavars.h"
#include "vdi_defs.h"

static MCS *mcs_ptr;

static void mouse_set_visible(WORD x, WORD y)
{
    linea_sprite_show_amiga(&mouse_cdb, mcs_ptr, x, y);
}

static void mouse_set_invisible(void)
{
    linea_sprite_hide_amiga(mcs_ptr);
}

static void mouse_move_to(WORD x, WORD y)
{
    linea_sprite_hide_amiga(mcs_ptr);
    linea_sprite_show_amiga(&mouse_cdb, mcs_ptr, x, y);
}

static void set_mouse_cursor(const MFORM *src)
{
    int i;
    WORD col;
    UWORD *gmdt;
    MCDB *dst = &mouse_cdb;
    const UWORD *mask;
    const UWORD *data;

    dst->xhot = src->mf_xhot & 0x000f;
    dst->yhot = src->mf_yhot & 0x000f;

    col = linea_validate_color_index(src->mf_bg);
    dst->bg_col = MAP_COL[col];

    col = linea_validate_color_index(src->mf_fg);
    dst->fg_col = MAP_COL[col];

    gmdt = dst->maskdata;
    mask = src->mf_mask;
    data = src->mf_data;
    for (i = 15; i >= 0; i--) {
        *gmdt++ = *mask++;
        *gmdt++ = *data++;
    }
}

static void resolution_changed(void)
{
#if EXTENDED_PALETTE
    mcs_ptr = (v_planes <= 4) ? &mouse_cursor_save : &ext_mouse_cursor_save;
#else
    mcs_ptr = &mouse_cursor_save;
#endif
}

const LINEA_MOUSE_RENDERER mouse_display_driver = {
    mouse_set_visible,
    mouse_set_invisible,
    mouse_move_to,
    set_mouse_cursor,
    resolution_changed
};

#endif /* MACHINE_AMIGA && CONF_WITH_SOFTWARE_MOUSE_RENDERING */
