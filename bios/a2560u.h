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
#include <stdbool.h>
#include "vicky2.h"
#include "foenix.h"

#define VIDEO_RAM_SIZE VRAM0_SIZE
#define BQ4802LY_BASE  (GAVIN+0x80)
#define PS2_BASE       (GAVIN+0x2800)
#define PS2_DATA       PS2_BASE
#define PS2_CMD        (PS2_BASE+0x04)

void a2560u_init(void); /* C entry point for initialisation */
void a2560u_debug(const char *, ...);
void a2560u_screen_init(void);
void a2560u_get_current_mode_info(uint16_t *planes, uint16_t *hz_rez, uint16_t *vt_rez);
void a2560u_setphys(const uint8_t *address);
void a2560u_set_border_color(uint32_t color);

/* Serial port */
uint32_t a2560u_bcostat1(void);
void a2560u_bconout1(uint8_t byte);

/* Time stuff */
void a2560u_xbtimer(uint16_t timer, uint16_t control, uint16_t data, void *vector);
void a2560u_set_timer(uint16_t timer, uint32_t frequency, bool repeat, void *handler);
void a2560u_timer_enable(uint16_t timer, bool enable);
void a2560u_clock_init(void);
uint32_t a2560u_getdt(void);
void a2560u_setdt(uint32_t datetime);

extern void *a2560_irq_vectors[IRQ_GROUPS][16];
void a2560U_irq_enable(uint16_t irq_id);
void a2560U_irq_disable(uint16_t irq_id);
void a2560u_irq_acknowledge(uint16_t irq_id);
void *a2560u_irq_set_handler(uint16_t irq_id, void *handler);

void a2560u_kbd_init(void);

#endif