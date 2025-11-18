#ifndef FOENIX_A2560U_H
#define FOENIX_A2560U_H

#include <stdint.h>
#include <stdbool.h>
#include "a2560_debug.h"
#include "a2560_struct.h"
#include "vicky2.h"
#include "vicky_mouse.h"
#include "foenix.h"


/* IDE support */
#if defined(MACHINE_A2560U) || defined(MACHINE_A2560K) || defined(MACHINE_A2560X) || defined(MACHINE_GENX)

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
#endif


void a2560_init(bool cold_boot);

void a2560_beeper(bool on);

#if defined(MACHINE_A2560M)
void a2560_sdc1_led(bool on);
void a2560_sdc2_led(bool on);
#else
void a2560_disk_led(bool on);
#endif

void a2560_system_info(struct foenix_system_info_t *result);

/* Video */
void a2560_setphys(const uint8_t *address);


void a2560_clock_init(void);
uint32_t a2560_getdt(void);
void a2560_setdt(uint32_t datetime);


void a2560_rte(void); /* Actually, just the RTE instruction. Not a function */
void a2560_rts(void); /* Actually, just the RTS instruction. */

void a2560_midi_init(uint32_t (*timer)(void),uint16_t timeout);

#endif
