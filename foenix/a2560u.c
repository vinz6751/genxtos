/*
 * a2560u - Foenix Retro Systems A2560U specific functions
 *
 *
 * Authors:
 *  VB   Vincent Barrilliot
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#define MACHINE_A2560U_DEBUG 0

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include "regutils.h"

int sprintf(char *__restrict__ str, const char *__restrict__ fmt, ...) __attribute__ ((format (printf, 2, 3)));

typedef signed char     SBYTE;                  /*  Signed byte         */
typedef unsigned char   UBYTE;                  /*  Unsigned byte       */
typedef unsigned long   ULONG;                  /*  unsigned 32 bit word*/
typedef int             BOOL;                   /*  boolean, TRUE or FALSE */
typedef short int       WORD;                   /*  signed 16 bit word  */
typedef unsigned short  UWORD;                  /*  unsigned 16 bit word*/
typedef long            LONG;                   /*  signed 32 bit word  */
#define MAKE_UWORD(hi,lo) (((UWORD)(UBYTE)(hi) << 8) | (UBYTE)(lo))
#define MAKE_ULONG(hi,lo) (((ULONG)(UWORD)(hi) << 16) | (UWORD)(lo))
#define LOWORD(x) ((UWORD)(ULONG)(x))
#define HIWORD(x) ((UWORD)((ULONG)(x) >> 16))
#define LOBYTE(x) ((UBYTE)(UWORD)(x))
#define HIBYTE(x) ((UBYTE)((UWORD)(x) >> 8))


#include "foenix.h"
#include "drivers/foenix_cpu.h"  /* Foenix's CPU */
#include "drivers/uart16550.h"   /* Serial port */
#include "drivers/sn76489.h"     /* Programmable Sound Generator */
#include "drivers/ym262.h"       /* YM262 OPL3 FM synthesizer */
#include "drivers/wm8776.h"      /* Audio codec */
#include "drivers/bq4802ly.h"    /* Real time clock */
#include "drivers/gavin_irq.h"   /* GAVIN-managed interrupts */
#include "drivers/gavin_timer.h" /* GAVIN's timers */
#include "ps2.h"
#include "ps2_keyboard.h"
#include "ps2_mouse_a2560u.h"
#include "drivers/vicky2.h"    /* VICKY II graphics controller */
#include "a2560u_debug.h"
#include "a2560u.h"

/* Prototypes ****************************************************************/

/* Interrupt */

void a2560u_irq_bq4802ly(void);
/* Interrupt */
void a2560u_irq_ps2kbd(void);
void a2560u_irq_ps2mouse(void);

/* Implementation ************************************************************/


void a2560u_init(void)
{
    cpu_freq = CPU_FREQ; /* TODO read that from GAVIN's Machine ID */

    gavin_irq_init();
    uart16550_init(UART0); /* So we can debug to serial port early */
    gavin_timer_init();
    wm8776_init();
    ym262_reset();
    sn76489_mute_all();
}


/* Video  ********************************************************************/

uint8_t *a2560u_bios_vram_fb; /* Address of framebuffer in video ram (from CPU's perspective) */


void a2560u_setphys(const uint8_t *address)
{
    a2560u_bios_vram_fb = (uint8_t*)address;
    vicky2_set_bitmap_address(0, (uint8_t*)((uint32_t)address - (uint32_t)VRAM_Bank0));
}






/* Real Time Clock  **********************************************************/

void a2560u_clock_init(void)
{
    cpu_set_vector(INT_BQ4802LY_VECN, a2560u_irq_bq4802ly);
    bq4802ly_init();
}


/* PS/2 setup  ***************************************************************/

/* List here all drivers we support */
static const struct ps2_driver_t * const drivers[] = {
    &ps2_keyboard_driver,
    &ps2_mouse_driver_a2560u
};


/* The following must be set by the calling OS prior to calling a2560u_kbd_init
 * ps2_config.counter      = ;
 * ps2_config.counter_freq = ;
 * ps2_config.malloc       = ;
 * ps2_config.os_callbacks.on_key_down = ;
 * ps2_config.os_callbacks.on_key_up   = ;
 * ps2_config.os_callbacks.on_mouse    = ;
 */
void a2560u_kbd_init(void)
{
    /* Disable IRQ while we're configuring */
    gavin_irq_disable(INT_KBD_PS2);
    gavin_irq_disable(INT_MOUSE);

    /* Explain our setup to the PS/2 subsystem */
    ps2_config.port_data    = (uint8_t*)PS2_DATA;
    ps2_config.port_status  = (uint8_t*)PS2_CMD;
    ps2_config.port_cmd     = (uint8_t*)PS2_CMD;
    ps2_config.n_drivers    = sizeof(drivers)/sizeof(struct ps2_driver_t*);
    ps2_config.drivers      = drivers;

    ps2_init();

    /* Register GAVIN interrupt handlers */
    cpu_set_vector(INT_PS2KBD_VECN, a2560u_irq_ps2kbd);
    cpu_set_vector(INT_PS2MOUSE_VECN, a2560u_irq_ps2mouse);

    /* Acknowledge any pending interrupt */
    gavin_irq_acknowledge(INT_KBD_PS2);
    gavin_irq_acknowledge(INT_MOUSE);

    /* Go ! */
    gavin_irq_enable(INT_KBD_PS2);
    gavin_irq_enable(INT_MOUSE);
}


/* System information ********************************************************/

static const uint32_t foenix_cpu_speed_hz[] =
{
    14318000,
    20000000,
    25000000,
    33000000,
    40000000,
    50000000,
    66000000,
    80000000
};

static const char* foenix_model_name[] =
{
    "C256 FMX",
    "C256 U",
    NULL,
    "A2560 dev",
    "Gen X",
    "C256 U+",
    NULL,
    NULL,
    "A2560X",
    "A2560U",
    NULL,
    "A2560K"
};

static const char * foenix_cpu_name[] =
{
    "MC68SEC000",
    "MC68020"
    "MC68EC020",
    "MC68030",
    "MC68EC30",
    "MC68040",
    "MC68040V",
};


void a2560u_system_info(struct foenix_system_info_t *result)
{
    result->fpga_date = MAKE_ULONG(R16(VICKY+0x30),R16(VICKY+0x32));
    result->fpga_major = R16(VICKY+0x3A);
    result->fpga_minor = R16(VICKY+0x38);
    result->fpga_partnumber = MAKE_ULONG(R16(VICKY+0x3E), R16(VICKY+0x3C));

    uint16_t revision = R16(VICKY+0x34);
    result->pcb_revision_name[0] = HIBYTE(revision);
    result->pcb_revision_name[1] = LOBYTE(revision);
    revision = R16(VICKY+0x36);
    result->pcb_revision_name[2] = HIBYTE(revision);
    result->pcb_revision_name[3] = '\0';

    result->cpu_name = (char*)foenix_cpu_name[R16(GAVIN+0x0C) >> 12];
    //result->model_name = foenix_model_name[poke]

    uint16_t machine_id = R16(GAVIN+0x0C);
    result->cpu_id = (machine_id & 0xf000) >> 12;
    result->cpu_name = (char*)foenix_cpu_name[result->cpu_id];
    result->cpu_speed_hz = foenix_cpu_speed_hz[(machine_id & 0xf00) >> 8];
    int model_id = machine_id & 0x0f;
    result->model_name = (char*)foenix_model_name[model_id];

    if (model_id == 9) /* A2560U */
    {
        if (((machine_id & 0xc0) >> 6) == 0x10)
            result->vram_size = (1L << 21); /* 2Mo */
        else
            result->vram_size = (1L << 22); /* 4Mo */

        // CPU speed not correctly reported ?
        result->cpu_speed_hz = foenix_cpu_speed_hz[1];
    }
}


void a2560u_beeper(bool on)
{
    if (on)
        R16(GAVIN_CTRL) |= GAVIN_CTRL_BEEPER;
    else
        R16(GAVIN_CTRL) &= ~GAVIN_CTRL_BEEPER;
}


void a2560u_disk_led(bool on)
{
    if (on)
        R16(GAVIN_CTRL) |= GAVIN_CTRL_DISKLED;
    else
        R16(GAVIN_CTRL) &= ~GAVIN_CTRL_DISKLED;
}
