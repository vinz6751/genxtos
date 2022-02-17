/*
 * linea.c - Line A functions and support
 *
 * Copyright 2022 The EmuTOS development team
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#ifndef LINE_A_H
#define LINE_A_H

#include "portab.h"
#include "lineavars.h"
#include "mform.h"

/* Convenience to say whether the palette is 256 entries rather than 16 */
#define EXTENDED_PALETTE (CONF_WITH_VIDEL || CONF_WITH_TT_SHIFTER || defined(MACHINE_A2560U))


/* Kind of driver definition for managing mouse display */
typedef struct {
    void (*mouse_set_visible)(WORD x, WORD y);
    void (*mouse_set_invisible)(void);
    void (*mouse_move_to)(WORD x, WORD y);
    void (*set_mouse_cursor)(const MFORM *src);
    void (*resolution_changed)(void);
} LINEA_MOUSE_RENDERER;

extern const LINEA_MOUSE_RENDERER mouse_display_driver;
extern void (*linea_on_resolution_changed)(void);

void linea_init(void); /* Initialize the Line-A layer */
void linea_resolution_changed(void);
void linea_mouse_init(void);
void linea_mouse_deinit(void);
void linea_mouse_force_show(void);
void linea_mouse_show(void);
void linea_mouse_hide(void);
void linea_mouse_transform(void);
void linea_set_screen_shift(void);    /* Set shift amount for screen address calcs */
void linea_mouse_set_form(const MFORM *src);
UWORD *get_start_addr(const WORD x, const WORD y);

/* Sprite support */
void linea_sprite_show_atari(MCDB *sprite, MCS *mcs, WORD x, WORD y);
void linea_sprite_hide_atari(MCS *mcs);

#endif

