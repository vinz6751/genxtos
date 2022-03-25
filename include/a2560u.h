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
#include "vicky2.h"
#include "foenix.h"


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
#define ide_interface ((volatile struct IDE*)IDE_BASE)


/* SD card support */
struct sdc_controller_t 
{
    uint8_t version;
    uint8_t control;
    uint8_t transfer_type;
    uint8_t transfer_control;
    uint8_t transfer_status;
    uint8_t transfer_error;
    uint8_t data;
    uint8_t sd_xxxa;
    uint8_t sd_xxax;
    uint8_t sd_xaxx;
    uint8_t sd_axxx;
    uint8_t spi_clock;
    uint8_t dummy0c[4];
    uint8_t rx_fifo_data;
    uint8_t dummy11[1];
    uint8_t rx_fifo_h;
    uint8_t rx_fifo_l;
    uint8_t rx_fifo_control;
    uint8_t dummy15[11];
    uint8_t tx_fifo;
    uint8_t dummy21[3];
    uint8_t tx_fifo_control;
};
#define sdc_controller ((volatile struct sdc_controller_t*)SDC_BASE)


/* System info from GAVIN and VICKY */
struct foenix_system_info_t
{
    char *model_name;
    uint8_t cpu_id;
    uint32_t cpu_speed_hz;
    uint32_t vram_size;
    char *cpu_name;
    char pcb_revision_name[4];
    uint32_t fpga_date; /* ddddmmyy, BCD */
    uint16_t fpga_major;
    uint16_t fpga_minor;
    uint32_t fpga_partnumber;
};


void a2560u_init(void); /* C entry point for initialisation */
void a2560u_beeper(bool on);
void a2560u_disk_led(bool on);
void a2560u_system_info(struct foenix_system_info_t *result);
void a2560u_debug(const char *, ...);
void a2560u_debugnl(const char* __restrict__ s, ...);

/* Video */
void a2560u_screen_init(void);
uint32_t a2560u_calc_vram_size(void);
void a2560u_mark_cell_dirty(const uint8_t *cell);
void a2560u_mark_screen_dirty(void);
void a2560u_get_current_mode_info(uint16_t *planes, uint16_t *hz_rez, uint16_t *vt_rez);
void a2560u_setphys(const uint8_t *address);
void a2560u_set_border_color(uint32_t color);

#define A2560U_DIRTY_CELLS_SIZE 1024
struct a2560u_dirty_cells_t
{
    uint16_t writer;
    uint16_t reader;
    uint16_t full_copy; /* Set this to non-zero to copy the whole frame buffer and flush the ring buffer */
    uint8_t *cells[A2560U_DIRTY_CELLS_SIZE];
};
extern volatile struct a2560u_dirty_cells_t a2560u_dirty_cells;

/* Serial port */
uint32_t a2560u_bcostat1(void);
void a2560u_bconout1(uint8_t byte);

/* Timing stuff */
#define HZ200_TIMER_NUMBER 2
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
void a2560u_irq_acknowledge(uint8_t irq_id);
void *a2560u_irq_set_handler(uint16_t irq_id, void *handler);

void a2560u_kbd_init(void);
void a2560u_update_cursor(void);

#endif /* MACHINE_A2560U */

#endif