/*
 * a2560u - Foenix Retro Systems A2560U specific functions
 *
 * Copyright (C) 2013-2021 The EmuTOS development team
 *
 * Authors:
 *  VB   Vincent Barrilliot
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#ifndef A2560U_H
#define A2560U_H

#include <stdint.h>
#include "vicky2.h"

#define VIDEO_RAM_SIZE VRAM0_SIZE

void a2560u_init(void); /* C entry point for initialisation */
void a2560u_screen_init(void);
void a2560u_get_current_mode_info(uint16_t *planes, uint16_t *hz_rez, uint16_t *vt_rez);
void a2560u_setphys(const uint8_t *address);
void a2560u_set_border_color(uint32_t color);

/* Serial port */
uint32_t a2560u_bcostat1(void);
void a2560u_bconout1(uint8_t byte);

#endif