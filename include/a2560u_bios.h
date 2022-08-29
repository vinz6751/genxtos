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

#ifdef MACHINE_A2560U

#include <stdint.h>
#include <stdbool.h>
#include "../foenix/vicky2.h"
#include "../foenix/a2560u.h"

/* C entry point for initialisation */
void a2560u_bios_init(void);
void a2560u_bios_screen_init(void);
void a2560u_bios_kbd_init(void);
void a2560u_bios_mark_cell_dirty(const uint8_t *cell_address);
void a2560u_bios_mark_screen_dirty(void);
uint32_t a2560u_bios_calc_vram_size(void);
void a2560u_bios_calibrate_delay(uint32_t calibration_time);
void a2560u_bios_text_init(void);

/* Serial port */
uint32_t a2560u_bios_bcostat1(void);
void a2560u_bios_bconout1(uint8_t byte);
void a2560u_bios_rs232_init(void);

/* Timing stuff */
#define HZ200_TIMER_NUMBER 2
void a2560u_bios_xbtimer(uint16_t timer, uint16_t control, uint16_t data, void *vector);

#endif /* MACHINE_A2560U */

#endif
