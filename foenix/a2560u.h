#ifndef FOENIX_A2560U_H
#define FOENIX_A2560U_H

#include <stdint.h>
#include <stdbool.h>
#include "a2560u_debug.h"
#include "foenix.h"
#include "drivers/vicky2.h"
#include "drivers/gavin_irq.h"
#include "drivers/foenix_cpu.h"

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
};
#define ide_interface ((volatile struct IDE* const)IDE_BASE)


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
#define sdc_controller ((volatile struct sdc_controller_t* const)SDC_BASE)


/* System info from GAVIN and VICKY */
struct foenix_system_info_t
{
    char     *model_name;
    uint8_t  cpu_id;
    uint32_t cpu_speed_hz;
    uint32_t vram_size;
    char     *cpu_name;
    char     pcb_revision_name[4];
    uint32_t fpga_date; /* ddddmmyy, BCD */
    uint16_t fpga_major;
    uint16_t fpga_minor;
    uint32_t fpga_partnumber;
};


void a2560u_init(void);

void a2560u_beeper(bool on);
void a2560u_disk_led(bool on);
void a2560u_system_info(struct foenix_system_info_t *result);

/* Video */
void a2560u_setphys(const uint8_t *address);


void a2560u_clock_init(void);


extern vector_t a2560_irq_vectors[IRQ_GROUPS][16];

void a2560u_kbd_init(void);

#endif
