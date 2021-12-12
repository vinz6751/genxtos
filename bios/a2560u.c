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

#define ENABLE_KDEBUG

#include <stdint.h>
#include <stdbool.h>

#include "emutos.h"
#include "vectors.h"
#include "tosvars.h"
#include "bios.h"
#include "processor.h"
#include "biosext.h"            /* for cache control routines */
#include "gemerror.h"
#include "ikbd.h"               /* for call_mousevec() */
#include "screen.h"
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

/* Local variables ***********************************************************/
static uint32_t cpu_freq; /* CPU frequency */
static uint32_t vbl_freq; /* VBL frequency */


/* Prototypes ****************************************************************/

void a2560u_init_lut0(void);
void a2560u_debug(const char *s);
void a2560u_serial_put(uint8_t b);

/* Interrupt */
static void irq_init(void);
static void timer_init(void);
void irq_add_handler(int id, void *handler);

/* Implementation ************************************************************/

void a2560u_init(void)
{
    *((unsigned long * volatile)0xB40008) = 0x0000000; /* Black border */

    cpu_freq = CPU_FREQ; /* TODO read that from GAVIN's Machine ID */

    irq_init();
    timer_init();
    uart16550_init(UART0); /* So we can debug to serial port early */    
    wm8776_init();
    sn76489_mute_all();

    /* Clear screen and home */
    KDEBUG(("\033E\r\n"));
}

void a2560u_screen_init(void)
{
    vicky2_init();
    vbl_freq = 60; /* TODO read that from VICKY's video mode */
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


/* Serial port support*/

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

void a2560u_serial_put(uint8_t b)
{
    uart16550_can_get(UART0);
}


/* For being able to translate settings of the ST's MFP68901 we need this */
static const uint16_t mfp_timer_prediv[] = { 0,4,10,16,50,64,100,200 };
#define MFP68901_FREQ 2457600 /* The ST's 68901 MFP is clocked at 2.4576MHZ */

void a2560u_xbtimer(uint16_t timer, uint16_t control, uint16_t data, void *vector)
{
    uint32_t frequency;
    uint32_t timer_clock;

    if (timer > 3)
        return;

    /* Read the intent of the caller */
    if (timer == 2) 
        control >>= 4; /* TCDCR has control bit for Timer C and D but D has bits 4,5,6 */
    if (control & 0x8)
    {
        /* We only support "delay mode. Other modes have bit 3 set */
        KDEBUG(("Not supported MFP timer %d mode %04x\n", 'A' + timer, control));
        return;
    }    
    /* Quantity of 2.4576MHz ticks before the timer fires */
    frequency = mfp_timer_prediv[frequency];
    if (data != 0)
        frequency *= data;
    
    /* Convert that according to our timer's clock */
    timer_clock = timer == 3 ? vbl_freq : cpu_freq;
    frequency = (frequency * timer_clock) / MFP68901_FREQ;

    a2560u_set_timer(timer, frequency, false, vector);
}

/* This holds all readily usable details of timers per timer number,
 * so we don't have to compute anything when setting a timer.
 * It's done so timers can be reprogrammed quickly without having to do
 * computation, so to get the best timing possible. */
const struct a2560u_timer_t a2560u_timers[] = 
{
    /* 0x24 is TIMER_CTRL_LOAD|TIMER_CTRL_RELOAD */
    { TIMER_CTRL0, TIMER0_VALUE, TIMER0_COMPARE, 0xffffff00, 0x000024, ~0x0100, INT_TIMER0_VECN },
    { TIMER_CTRL0, TIMER1_VALUE, TIMER1_COMPARE, 0xffff00ff, 0x002400, ~0x0200, INT_TIMER1_VECN },
    { TIMER_CTRL0, TIMER2_VALUE, TIMER2_COMPARE, 0xff00ffff, 0x240000, ~0x0400, INT_TIMER2_VECN },
    { TIMER_CTRL1, TIMER3_VALUE, TIMER3_COMPARE, 0xffffff00, 0x000024, ~0x0800, INT_TIMER3_VECN }
};


/*
 * Initialise the timers, including the 200Hz system timer.
 */
static void timer_init(void) 
{
    int i;

    /* Disable all timers */
    for (i = 0; i < 4; i++)
        a2560u_timer_enable(i, false);
}

/*
 * Start a timer.
 * timer: timer number
 * frequency: desired frequency
 * repeat: whether the timer should reload after firing interrupt
 * handler: routine to execute when timer fires
 */
void a2560u_set_timer(uint16_t timer, uint32_t frequency, bool repeat, void *handler)
{
    struct a2560u_timer_t *t;
    uint16_t sr;

    if (timer > 3)
        return;

    KDEBUG(("\033ESet timer:%d, %ld, %d, %p\n",timer,frequency,repeat,handler));
    
    /* Identify timer control register to use */
    t = (struct a2560u_timer_t *)&a2560u_timers[timer];

    /* We don't want interrupts while we reprogramming */
    sr = set_sr(0x2700);

    /* Stop the timer while we configure */    
    a2560u_timer_enable(timer, false);

    /* Set timer period */
    //KDEBUG(("cpu_freq=%ld frequency=%ld, cpu/freq=%p\n", cpu_freq, frequency, (void*)(cpu_freq / frequency)));
    *(t->value) = (cpu_freq / frequency);
    *(t->compare) = 0L;

    /* Set handler */    
    setexc(t->vector, (uint32_t)handler);

    /* Before starting the timer, ignore any previous pending interrupt from it */
    R16(IRQ_PENDING_GRP1) &= t->irq_mask;

    /* Unmask interrupts for that timer */
    R16(IRQ_MASK_GRP1) &= t->irq_mask;

    /* Start the timer.
     * TODO: In case of control register 0, we write the config of 3 timers at once:
     * can that have nasty effects on any other running timers ? */    
    KDEBUG(("CPU sr=%04x (before starting timer)\n",get_sr()));    

    KDEBUG(("BEFORE STARTING TIMER\n"));
    KDEBUG(("CPU sr=%04x\n",get_sr()));
    KDEBUG(("value           %p=%p\n", t->value,(void*)*t->value));
    KDEBUG(("irq_pending(1)  %p=%04x\n", (void*)IRQ_PENDING_GRP1,R16(IRQ_PENDING_GRP1)));
    KDEBUG(("irq_polarity(1) %p=%04x\n", (void*)IRQ_POL_GRP1,R16(IRQ_POL_GRP1)));
    KDEBUG(("irq_edge(1)     %p=%04x\n", (void*)IRQ_EDGE_GRP1,R16(IRQ_EDGE_GRP1)));
    KDEBUG(("irq_mask(1)     %p=%04x\n", (void*)IRQ_MASK_GRP1,R16(IRQ_MASK_GRP1)));    
    KDEBUG(("control         %p=%p\n",t->control,(void*)*t->control));

    *(t->control) |= t->start;
    set_sr(sr);
    KDEBUG(("AFTER STARTING TIMER\n"));
    KDEBUG(("CPU sr=%04x (after starting timer)\n",get_sr()));
    KDEBUG(("value           %p=%p\n", t->value,(void*)*t->value));
    KDEBUG(("irq_pending(1)  %p=%04x\n", (void*)IRQ_PENDING_GRP1,R16(IRQ_PENDING_GRP1)));
    KDEBUG(("irq_polarity(1) %p=%04x\n", (void*)IRQ_POL_GRP1,R16(IRQ_POL_GRP1)));
    KDEBUG(("irq_edge(1)     %p=%04x\n", (void*)IRQ_EDGE_GRP1,R16(IRQ_EDGE_GRP1)));
    KDEBUG(("irq_mask(1)     %p=%04x\n", (void*)IRQ_MASK_GRP1,R16(IRQ_MASK_GRP1)));    
    KDEBUG(("control         %p=%p\n",t->control,(void*)*t->control));

    KDEBUG(("After start.... charge=%p charge=%p\n",(void*)(*t->value),(void*)(*t->value)));

    //a2560u_debug("TIMER RUNNING\r\n");
    R32(0xb40008) = 0xffff0000;//for(;;);
}

/*
 * Enable or disable a timer.
 * timer: timer number
 * enable: 0 to disable, anything to enable
 */
void a2560u_timer_enable(uint16_t timer, bool enable)
{
    struct a2560u_timer_t *t = (struct a2560u_timer_t *)&a2560u_timers[timer];
    if (enable)
        *t->control |= TIMER_CTRL_ENABLE;
    else
        *t->control &= ~TIMER_CTRL_ENABLE;
}


/*
 * Output the string on the serial port
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
    *border = 0x0000ff00;
}

/* Interrupts management */
static void irq_init(void)
{
    int i;

    /* This is only during startup, in normal user you would never disable all interrupts. Would you ? */
    volatile uint16_t *pending = (uint16_t*)IRQ_PENDING_GRP0;
    volatile uint16_t *polarity = (uint16_t*)IRQ_POL_GRP0;
    volatile uint16_t *edge = (uint16_t*)IRQ_EDGE_GRP0;
    volatile uint16_t *mask = (uint16_t*)IRQ_MASK_GRP0;
    
    // FIXME
    for (i = 0; i < IRQ_GROUPS; i++)
    {
        pending[i] = polarity[i] = 0;
        mask[i] = 0xffff;
        edge[i] = 0xffff;
    }

    for (i=0x40; i<0x60; i++)
        setexc(i,(uint32_t)just_rte);    
}
