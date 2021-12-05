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

/* #define ENABLE_KDEBUG */

#include "emutos.h"
#include "amiga.h"
#include "vectors.h"
#include "tosvars.h"
#include "bios.h"
#include "processor.h"
#include "biosext.h"            /* for cache control routines */
#include "gemerror.h"
#include "ikbd.h"               /* for call_mousevec() */
#include "screen.h"
#include "videl.h"
#include "delay.h"
#include "asm.h"
#include "string.h"
#include "disk.h"
#include "biosmem.h"
#include "bootparams.h"
#include "machine.h"
#include "has.h"
#include "../bdos/bdosstub.h"
#include "screen.h"
#include "stdint.h"
#include "foenix.h"
#include "uart16550.h" /* Serial port */
#include "sn76489.h"   /* Programmable Sound Generator */
#include "wm8776.h"    /* Audio codec */
#include "vicky2.h"    /* VICKY II graphics controller */
#include "a2560u.h"

/* Prototypes ****************************************************************/

void a2560u_init_lut0(void);
void a2560u_debug(const char *s);
void a2560u_serial_put(uint8_t b);

static void disable_interrupts(void);


void a2560u_init(void)
{
    *((unsigned long * volatile)0xB40008) = 0x0000000;
    
    disable_interrupts();
    uart16550_init(UART0); /* So we can debug to serial port early */    
    wm8776_init();
    sn76489_mute_all();
    /* Clear screen and home */
    KDEBUG(("\033E\r\n"));
}

void a2560u_screen_init(void)
{
    vicky2_init();
}

void a2560u_setphys(const uint8_t *address)
{
    vicky2_set_bitmap0_address(address-VRAM_Bank0);
}

void a2560u_set_border_color(uint32_t color)
{
    vicky2_set_border_color(color);
}

void a2560u_get_current_mode_info(UWORD *planes, UWORD *hz_rez, UWORD *vt_rez)
{
    FOENIX_VIDEO_MODE mode;
    vicky2_get_video_mode(&mode);
    *planes = 8;
    *hz_rez = mode.w;
    *vt_rez = mode.h;
}

uint32_t a2560u_bcostat1(void)
{
    return uart16550_can_put(UART0);
}

void a2560u_bconout1(uint8_t byte)
{
    vicky2_set_border_color(0x000000);
    uart16550_put(UART0, &byte, 1);
    vicky2_set_border_color(0x666666);
}

static void disable_interrupts(void)
{
    /* This is only during startup, in normal user you would never disable all interrupts. Would you ? */
    int i;
    uint16_t *pending = (uint16_t*)IRQ_PENDING_GRP0;
    uint16_t *polarity = (uint16_t*)IRQ_POL_GRP0;
    uint16_t *edge = (uint16_t*)IRQ_EDGE_GRP0;
    uint16_t *mask = (uint16_t*)IRQ_MASK_GRP0;
    
    for (i = 0; i < IRQ_GROUPS; i++)
    {
        pending[i] = polarity[i] = 0;
        edge[i] = mask[i] = 0xffff;
    }
}


void a2560u_serial_put(uint8_t b)
{
    uart16550_can_get(UART0);
}

/* Output the string on the serial port
 * Use this if the KDEBUG is not available yet.
 */
void a2560u_debug(const char *s)
{
    char *c = (char*)s;
    long *border = (long*)0xb40008;
    *border = 0x00ffffff;
    while (*c)
    {
        a2560u_serial_put(*c);
        *border = 0x000000f0;
    }
    //*border = 0x00ff0000;
}
