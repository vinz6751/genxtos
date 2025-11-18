/*
 * a2560 - Foenix Retro Systems A2560U specific functions
 *
 * Copyright (C) 2013-2023 The EmuTOS development team
 *
 * Authors:
 *  VB   Vincent Barrilliot
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#define MACHINE_A2560_DEBUG 1

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include "a2560_debug.h"

#include "vicky2_txt_a_logger.h"

/* FIXME: this is an EmuTOS dependency */
#include "../include/doprintf.h"

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
#include "a2560_debug.h"
#include "a2560.h"
#include "bq4802ly.h"  /* Real time clock */
#include "cpu.h"
#include "interrupts.h"
#include "keyboard.h"
#include "mpu401.h"    /* MIDI interface */
#include "regutils.h"
#include "sn76489.h"   /* Programmable Sound Generator */
#include "superio.h"
#include "timer.h"
#include "uart16550.h" /* Serial port */
#include "vicky2.h"    /* VICKY II graphics controller */
#include "wm8776.h"    /* Audio codec */
#include "ym262.h"     /* YM262 OPL3 FM synthesizer */


/* Local variables ***********************************************************/
uint32_t cpu_freq; /* CPU frequency */


/* Prototypes ****************************************************************/

void a2560_init_lut0(void);


void a2560_irq_bq4802ly(void);


/* Implementation ************************************************************/

void a2560_init(bool cold_boot)
{
    a2560_debugnl("a2560_init()");

#if !defined(MACHINE_A2560M)
    a2560_beeper(true);
#endif

    cpu_freq = CPU_FREQ; /* TODO read that from GAVIN's Machine ID */

	/* Init SuperIO (if there) and serial ports so we can send debug output early */
#if defined(MACHINE_A2560K) || defined(MACHINE_A2560M) || defined(MACHINE_A2560X) || defined(MACHINE_GENX)
    a2560_debugnl("superio_init()");
    superio_init();

    a2560_debugnl("uart16550_init()");
    uart16550_init((UART16550*)UART1);
    uart16550_init((UART16550*)UART2);
#elif defined(MACHINE_A2560U)
    uart16550_init((UART16550*)UART1);
#endif

    a2560_debugnl("a2560_irq_init()");
    a2560_irq_init();
	
    a2560_debugnl("a2560_timer_init");
    a2560_timer_init();

    a2560_debugnl("wm8776_init");
    wm8776_init();

#if !defined(MACHINE_A2560M) /* Not supported yet */
    a2560_debugnl("ym262_reset");
    ym262_reset();
#endif

    a2560_debugnl("sn76489_mute_all");
    sn76489_mute_all();

    /* Clear screen and home */
    a2560_debugnl("a2560_init() done");

#if !defined(MACHINE_A2560M)
    a2560_beeper(false);
#endif
}


/* Video  ********************************************************************/

uint8_t *a2560_bios_vram_fb; /* Address of framebuffer in video ram (from CPU's perspective) */


void a2560_setphys(const uint8_t *address)
{
    a2560_bios_vram_fb = (uint8_t*)address;
    vicky2_set_bitmap_address(vicky, 0, (uint8_t*)((uint32_t)address - (uint32_t)VRAM_Bank0));
}


/* Serial port support *******************************************************/

void a2560_irq_com1(void); /* Event handler in a2560_s.S */


/* Real Time Clock  **********************************************************/

void a2560_clock_init(void)
{
    cpu_set_vector(INT_BQ4802LY_VECN, (uint32_t)a2560_irq_bq4802ly);
    bq4802ly_init();
}


uint32_t a2560_getdt(void)
{
    uint8_t day, month, hour, minute, second;
    uint16_t year;

    bq4802ly_get_datetime(&day, &month, &year, &hour, &minute, &second);
    a2560_debugnl("RTC time= %02d/%02d/%04d %02d:%02d:%02d", day, month, year, hour, minute, second);

    return MAKE_ULONG(
        ((year-1980) << 9) | (month << 5) | day,
        (hour << 11) | (minute << 5) | (second>>1)/*seconds are of units of 2*/);
}


void a2560_setdt(uint32_t datetime)
{
    const uint16_t date = HIWORD(datetime);
    const uint16_t time = LOWORD(datetime);

    bq4802ly_set_datetime(
        (date & 0b0000000000011111),       /* day */
        (date & 0b0000000111100000) >> 5,  /* month */
        ((date & 0b1111111000000000) >> 9) + 1980,  /* year */
        (time & 0b1111100000000000) >> 11, /* hour */
        (time & 0b0000011111100000) >> 5,  /* minute */
        (time & 0b0000000000011111) << 1); /* second are divided by 2 in TOS*/
}


/* MIDI **********************************************************************/
#if CONF_WITH_MPU401
extern void a2560_irq_mpu401(void);

void (*mpu401_rx_handler)(uint8_t byte);

void a2560_midi_init(uint32_t (*timer)(void),uint16_t timeout) {
    a2560_debugnl("a2560_midi_init");
    a2560_irq_disable(INT_MIDI);
    mpu401_set_timeout(timer, timeout);
    mpu401_rx_handler = (void(*)(uint8_t))a2560_rts;
    cpu_set_vector(INT_MIDI_VECN, (uint32_t)a2560_irq_mpu401);
    a2560_irq_acknowledge(INT_MIDI);
    a2560_irq_enable(INT_MIDI);
    a2560_debugnl("mpu401_init returns %d",mpu401_init());
}

#endif /* CONF_WITH_MPU401 */

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

#define FOENIX_MODEL_NAME_SIZE 12
static const char* foenix_model_name[12] =
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

#define FOENIX_CPU_NAME_SIZE 11
static const char * foenix_cpu_name[FOENIX_CPU_NAME_SIZE] =
{
    "MC68SEC000",
    "MC68020"
    "MC68EC020",
    "MC68030",
    "MC68EC30",
    "MC68040",
    "MC68040V",
    "MC68EC40",
    "486DX2-50",
    "486DX2-66",
    "486DX4"
};


void a2560_system_info(struct foenix_system_info_t *result)
{
    /* From VICKY */
#ifdef MACHINE_A2560U
    result->fpga_date = MAKE_ULONG(R16(VICKY+0x30),R16(VICKY+0x32));
    result->fpga_major = R16(VICKY+0x3A);
    result->fpga_minor = R16(VICKY+0x38);
    result->fpga_partnumber = MAKE_ULONG(R16(VICKY+0x3E), R16(VICKY+0x3C));

    uint16_t revision = R16(VICKY+0x34);
    result->pcb_revision_name[0] = HIBYTE(revision);
    result->pcb_revision_name[1] = LOBYTE(revision);
    revision = R16(VICKY+0x36);
    result->pcb_revision_name[2] = HIBYTE(revision);

#elif defined(MACHINE_A2560K) || defined(MACHINE_A2560M) || defined(MACHINE_A2560X) || defined(MACHINE_GENX)
    result->fpga_date = R32(VICKY+0x14);
    result->fpga_major = (uint16_t)(R32(VICKY+0x10) >> 16);
    result->fpga_major = (uint16_t)(R32(VICKY+0x10) & 0xffff);
    uint32_t revision = R32(VICKY+0x18);
    result->pcb_revision_name[0] = (revision >> 24);
    result->pcb_revision_name[1] = (revision >> 16);
    result->pcb_revision_name[2] = (revision >> 8);
#else
    #error "Foenix model not recognized"
#endif
    result->pcb_revision_name[3] = '\0';

    /* From GAVIN */
    uint16_t machine_id = GAVIN_R(GAVIN+0x0C);
    
    a2560_debugnl("Machine id: %x", machine_id);
    result->cpu_id = (machine_id >> 12) & 7;
    if (result->cpu_id >= FOENIX_CPU_NAME_SIZE)
        result->cpu_name = "Unknown";
    else
        result->cpu_name = (char*)foenix_cpu_name[result->cpu_id];

    result->cpu_speed_hz = foenix_cpu_speed_hz[(machine_id >> 5) & 7];
    
    int model_id = machine_id & 0x0f;
    if (model_id >= FOENIX_MODEL_NAME_SIZE)
        result->model_name = "Unknown";
    else
        result->model_name = (char*)foenix_model_name[model_id];

    if (model_id == 9) /* A2560U */
    {
        if (((machine_id & 0xc0) >> 6) == 0x10)
            result->vram_size = (1L << 21); /* 2Mo */
        else
            result->vram_size = (1L << 22); /* 4Mo */

        /* CPU speed not correctly reported ? */
        result->cpu_speed_hz = foenix_cpu_speed_hz[1];
    }

#if 0 /* To produce EmuTOS "advertisement builds" while FPGA is not finished */
    result->cpu_name = "MC68LC060";
    result->cpu_speed_hz = 33333333;
    result->model_name = "A2560M FOENIX";
#endif
}


void a2560_beeper(bool on)
{
    if (on)
        GAVIN_R(GAVIN_CTRL) |= GAVIN_CTRL_BEEPER;
    else
        GAVIN_R(GAVIN_CTRL) &= ~GAVIN_CTRL_BEEPER;
}

#if defined(MACHINE_A2560M)
void a2560_sdc1_led(bool on)
{
    if (on)
        GAVIN_R(GAVIN_CTRL) |= GAVIN_CTRL_SDC1_LED;
    else
        GAVIN_R(GAVIN_CTRL) &= ~GAVIN_CTRL_SDC1_LED;
}
void a2560_sdc2_led(bool on)
{
    if (on)
        GAVIN_R(GAVIN_CTRL) |= GAVIN_CTRL_SDC2_LED;
    else
        GAVIN_R(GAVIN_CTRL) &= ~GAVIN_CTRL_SDC2_LED;
}
#else
void a2560_disk_led(bool on)
{
    if (on)
        GAVIN_R(GAVIN_CTRL) |= GAVIN_CTRL_DISKLED;
    else
        GAVIN_R(GAVIN_CTRL) &= ~GAVIN_CTRL_DISKLED;
}
#endif
