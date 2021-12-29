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

/* IDE support */
struct IDE
{   
    uint16_t data;
    uint8_t features; /* Read: error */    
    uint8_t filler03[1];
    uint8_t sector_count;
    uint8_t filler05[1];
    uint8_t sector_number;
    uint8_t filler07[1];
    uint8_t cylinder_low;
    uint8_t filler09[1];
    uint8_t cylinder_high;
    uint8_t filler0b[1];
    uint8_t head;
    uint8_t filler0d[1];
    uint8_t command; /* Read: status */
    uint8_t filler0f[1];
    uint8_t control;  /* Read: Alternate status */

#if 0    
    UBYTE filler1e[4091];
    UBYTE control; /* Read: Alternate status */
    UBYTE filler1019[3];
    UBYTE address; /* Write: Not used */
    UBYTE filler02[4067];
#endif
};
#define ide_interface ((volatile struct IDE*)(GAVIN+0x400))


void a2560u_init(void); /* C entry point for initialisation */
void a2560u_debug(const char *, ...);
void a2560u_screen_init(void);
void a2560u_get_current_mode_info(uint16_t *planes, uint16_t *hz_rez, uint16_t *vt_rez);
void a2560u_setphys(const uint8_t *address);
void a2560u_set_border_color(uint32_t color);

/* Serial port */
uint32_t a2560u_bcostat1(void);
void a2560u_bconout1(uint8_t byte);

/* Timing stuff */
#define HZ200_TIMER_NUMBER 1 /* There is a problem with timer 2 */
void a2560u_xbtimer(uint16_t timer, uint16_t control, uint16_t data, void *vector);
void a2560u_set_timer(uint16_t timer, uint32_t frequency, bool repeat, void *handler);
void a2560u_timer_enable(uint16_t timer, bool enable);
void a2560u_clock_init(void);
uint32_t a2560u_getdt(void);
void a2560u_setdt(uint32_t datetime);
void a2560u_calibrate_delay(uint32_t calibration_time);


extern void *a2560_irq_vectors[IRQ_GROUPS][16];
/* Backup all IRQ mask registers and mask all interrupts */
void a2560u_irq_mask_all(uint16_t *save);
/* Restore interrupts backed up with a2560u_irq_mask_all */
void a2560u_irq_restore(const uint16_t *save);
void a2560u_irq_enable(uint16_t irq_id);
void a2560u_irq_disable(uint16_t irq_id);
void a2560u_irq_acknowledge(uint16_t irq_id);
void *a2560u_irq_set_handler(uint16_t irq_id, void *handler);

void a2560u_kbd_init(void);

#endif