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
#include "uart16550.h" /* Serial port */
#include "sn76489.h"   /* Programmable Sound Generator */
#include "ym262.h"     /* YM262 OPL3 FM synthesizer */
#include "wm8776.h"    /* Audio codec */
#include "bq4802ly.h"  /* Real time clock */
#include "ps2.h"
#include "ps2_keyboard.h"
#include "ps2_mouse_a2560u.h"
#include "vicky2.h"    /* VICKY II graphics controller */
#include "a2560u_debug.h"
#include "a2560u.h"

#define set_sr(a)                         \
__extension__                             \
({short _r, _a = (a);                     \
  __asm__ volatile                        \
  ("move.w sr,%0\n\t"                     \
   "move.w %1,sr"                         \
  : "=&d"(_r)        /* outputs */        \
  : "nd"(_a)         /* inputs  */        \
  : "cc", "memory"   /* clobbered */      \
  );                                      \
  _r;                                     \
})

/* Local variables ***********************************************************/
static uint32_t cpu_freq; /* CPU frequency */


/* Prototypes ****************************************************************/

void a2560u_init_lut0(void);
static void timer_init(void);
static uint32_t set_vector(uint16_t num, uint32_t vector);
/* Interrupt */
static void irq_init(void);
void irq_mask_all(uint16_t *save);
void irq_add_handler(int id, void *handler);
void a2560u_irq_bq4802ly(void);
void a2560u_irq_vicky(void);
void a2560u_irq_ps2kbd(void);
void a2560u_irq_ps2mouse(void);

/* Implementation ************************************************************/


void a2560u_init(void)
{
    a2560u_beeper(true);
    *((unsigned long * volatile)0xB40008) = 0x0000000; /* Black border */

    cpu_freq = CPU_FREQ; /* TODO read that from GAVIN's Machine ID */

    irq_init();
    uart16550_init(UART0); /* So we can debug to serial port early */
    timer_init();
    wm8776_init();
    ym262_reset();
    sn76489_mute_all();

    /* Clear screen and home */
    a2560u_debugnl("\033Ea2560u_init()");
    a2560u_beeper(false);
}


/* Video  ********************************************************************/

uint8_t *a2560u_bios_vram_fb; /* Address of framebuffer in video ram (from CPU's perspective) */


void a2560u_setphys(const uint8_t *address)
{
    a2560u_bios_vram_fb = (uint8_t*)address;
    vicky2_set_bitmap_address(0, (uint8_t*)((uint32_t)address - (uint32_t)VRAM_Bank0));
}


/* Serial port support *******************************************************/

void a2560u_irq_com1(void); // Event handler in a2560u_s.S


/* Timers ********************************************************************/

/* This holds all readily usable details of timers per timer number,
 * so we don't have to compute anything when setting a timer.
 * It's done so timers can be reprogrammed quickly without having to do
 * computation, so to get the best timing possible. */
/* Timers */
static const struct a2560u_timer_t {
    uint32_t control;  /* Control register */
    uint32_t value;    /* Value register   */
    uint32_t compare;  /* Compare register */
    uint32_t deprog;   /* AND the control register with this to clear all settings */
    uint32_t reset;    /* OR the control register with this to reset value/compare */
    uint32_t prog;     /* OR the control register with this to program the timer in countdown mode */
    uint32_t start;    /* OR the control register with this to start the timer in countdown mode */
    uint16_t irq_mask; /* OR this to the irq_pending_group to acknowledge the interrupt */
    uint16_t vector;   /* Exception vector number (not address !) */
    uint32_t dummy;    /* Useless but having the structure 32-byte larges makes it quicker to generate an offset with lsl #5 */
} a2560u_timers[] =
{
/* Whether to count up or count down. Both work, it really doesn't make a difference */
#define TIMER_COUNT_UP 1
#if TIMER_COUNT_UP
    #define TIMER_PROG  (TIMER_CTRL_IRQ|TIMER_CTRL_RECLEAR|TIMER_CTRL_UPDOWN)
    #define TIMER_RESET TIMER_CTRL_CLEAR
#else
    #define TIMER_PROG (TIMER_CTRL_IRQ|TIMER_CTRL_RELOAD)
    #define TIMER_RESET TIMER_CTRL_LOAD
#endif
    /* TODO 1L as "start" is really TIMER_CTRL_ENABLE but I get a "left shift count >= width of type" warning I can't get rid of */
    { TIMER_CTRL0, TIMER0_VALUE, TIMER0_COMPARE, 0xffffff00, TIMER_RESET << 0,  TIMER_PROG << 0,  TIMER_CTRL_ENABLE << 0,  0x0100, INT_TIMER0_VECN },
    { TIMER_CTRL0, TIMER1_VALUE, TIMER1_COMPARE, 0xffff00ff, TIMER_RESET << 8,  TIMER_PROG << 8,  TIMER_CTRL_ENABLE << 8,  0x0200, INT_TIMER1_VECN },
    { TIMER_CTRL0, TIMER2_VALUE, TIMER2_COMPARE, 0xff00ffff, TIMER_RESET << 16, TIMER_PROG << 16, TIMER_CTRL_ENABLE << 16, 0x0400, INT_TIMER2_VECN },
    { TIMER_CTRL1, TIMER3_VALUE, TIMER3_COMPARE, 0xffffff00, TIMER_RESET << 0,  TIMER_PROG << 0,  TIMER_CTRL_ENABLE << 0,  0x0800, INT_TIMER3_VECN }
};


/*
 * Initialise the timers.
 */
static void timer_init(void)
{
    int i;

    /* Disable all timers */
    for (i = 0; i < sizeof(a2560u_timers)/sizeof(struct a2560u_timer_t); i++)
        a2560u_timer_enable(i, false);
}

/*
 * Program a timer but don't start it. This causes the timer to stop.
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

    //a2560u_debugnl("Set timer %d, freq:%ldHz, repeat:%s, handler:%p\n",timer,frequency,repeat?"ON":"OFF",handler);

    /* Identify timer control register to use */
    t = (struct a2560u_timer_t *)&a2560u_timers[timer];

    /* We don't want interrupts while we reprogramming */
    sr = set_sr(0x2700);

    /* Stop the timer while we configure */
    a2560u_timer_enable(timer, false);

    /* Stop and reprogram the timer, but don't start. */
    /* TODO: In case of control register 0, we write the config of 3 timers at once because
     * the timers control register has to be addressed as long word.
     * Can that have nasty effects on any other running timers ? */
    R32(t->control) = R32(t->control) & t->deprog;

    /* Set timer period */
#if TIMER_COUNT_UP
    R32(t->compare) = cpu_freq / frequency;
# define TIMER_USE_OFFICIAL_WAY 0
# if TIMER_USE_OFFICIAL_WAY
    R32(t->control) |= t->reset;
    R32(t->control) &= ~t->reset;
# else
    /* This works just as well and is a quicker */
    R32(t->value) = 0L;
# endif

#else
    R32(t->value) = cpu_freq / frequency;
    R32(t->compare) = 0L;
#endif

    /* When counting up, the Value register is cleared whenever CLEAR **OR** RECLEAR is set.
     * Likewise when counting down, Value register is loaded whenever LOAD **OR** RELOAD is set. */

    R32(t->control) |= t->prog;

    /* Set handler */    
    set_vector(t->vector, (uint32_t)handler);

    /* Before starting the timer, ignore any previous pending interrupt from it */
    if (R16(IRQ_PENDING_GRP1) & t->irq_mask)
        R16(IRQ_PENDING_GRP1) = t->irq_mask; /* Yes it's an assignment, no a OR. That's how interrupts are acknowledged */

    /* Unmask interrupts for that timer */
    R16(IRQ_MASK_GRP1) &= ~t->irq_mask;

    set_sr(sr);
#if 0
    a2560u_debugnl("AFTER SETTING TIMER %d\n", timer);
    a2560u_debugnl("CPU          sr=%04x\n", get_sr());
    a2560u_debugnl("vector       0x%02x=%p\n",t->vector,(void*)set_vector(t->vector, -1L));
    a2560u_debugnl("value        %p=%08lx\n",(void*)t->value,R32(t->value));
    a2560u_debugnl("irq_pending  %p=%04x\n", (void*)IRQ_PENDING_GRP1,R16(IRQ_PENDING_GRP1));
    a2560u_debugnl("irq_mask     %p=%04x\n", (void*)IRQ_MASK_GRP1,R16(IRQ_MASK_GRP1));
    a2560u_debugnl("control      %p=%08lx\n",(void*)t->control,R32(t->control));
#endif
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
        R32(t->control) |= t->start;
    else
        R32(t->control) &= ~t->start;

// a2560u_debugnl("After %s  > control %p=0x%08lx\n",enable?"Enable":"Disable", (void*)t->control,R32(t->control));
//     if (R32(t->value) != R32(t->value))
//         a2560u_debugnl("Timer is running: 0x%08lx 0x%08lx 0x%08lx...\n",R32(t->value), R32(t->value), R32(t->value));
//     else
//         a2560u_debugnl("Timer is not running\n");
}


/*
 * Output the string on the serial port.
 */
void a2560u_debugnl(const char* __restrict__ s, ...)
{
#if MACHINE_A2560U_DEBUG
    char msg[80];
    int length;
    char *c;
    va_list ap;
    va_start(ap, s);
    sprintf(msg,s,ap);
    va_end(ap);

    c = (char*)msg;
    while (*c++)
        ;
    length = c - msg -1;

    uart16550_put(UART0, (uint8_t*const)msg, length);
    uart16550_put(UART0, (uint8_t*)"\r\n", 2);
#endif
}

void a2560u_debug(const char* __restrict__ s, ...)
{
#if MACHINE_A2560U_DEBUG
    char msg[80];
    int length;
    char *c;
    va_list ap;
    va_start(ap, s);
    sprintf(msg,s,ap);
    va_end(ap);

    c = (char*)msg;
    while (*c++)
        ;
    length = c - msg -1;

    uart16550_put(UART0, (uint8_t*const)msg, length);
    uart16550_put(UART0, (uint8_t*)"\r\n", 2);
#endif
}


/* Interrupts management *****************************************************/

/* Interrupt handlers for each of the IRQ groups */
void *a2560_irq_vectors[IRQ_GROUPS][16];

static void irq_init(void)
{
    int i, j;

    volatile uint16_t *pending = (uint16_t*)IRQ_PENDING_GRP0;
    volatile uint16_t *polarity = (uint16_t*)IRQ_POL_GRP0;
    volatile uint16_t *edge = (uint16_t*)IRQ_EDGE_GRP0;
    volatile uint16_t *mask = (uint16_t*)IRQ_MASK_GRP0;

    for (i = 0; i < IRQ_GROUPS; i++)
    {
        mask[i] = edge[i] = 0xffff;
        pending[i] = 0xffff; /* Acknowledge any pending interrupt */
        polarity[i] = 0;

        for (j=0; j<16; j++)
            a2560_irq_vectors[i][j] = a2560u_rts;
    }

    for (i=0x40; i<0x60; i++)
        set_vector(i,(uint32_t)a2560u_rte); /* That's not even correct because it doesn't acknowledge interrupts */

    set_vector(INT_VICKYII, (uint32_t)a2560u_irq_vicky);
}


void a2560u_irq_mask_all(uint16_t *save)
{
    int i;
    uint16_t sr = set_sr(0x2700);

    for (i = 0; i < IRQ_GROUPS; i++)
    {
        save[i] = ((volatile uint16_t*)IRQ_MASK_GRP0)[i];
        ((volatile uint16_t*)IRQ_MASK_GRP0)[i] = 0xffff;
    }

    set_sr(sr);
}


void a2560u_irq_restore(const uint16_t *save)
{
    int i;

    for (i = 0; i < IRQ_GROUPS; i++)
        ((volatile uint16_t*)IRQ_MASK_GRP0)[i] = save[i];
}

/* Utility functions, don't use directly */
static inline uint16_t irq_group(uint16_t irq_id) { return irq_id >> 4; }
static inline uint16_t irq_number(uint16_t irq_id) { return irq_id & 0xf; }
static inline uint16_t irq_mask(uint16_t irq_id) { return 1 << irq_number(irq_id); }
static inline uint16_t *irq_mask_reg(uint16_t irq_id) { return &((uint16_t*)IRQ_MASK_GRP0)[irq_group(irq_id)]; }
static inline uint16_t *irq_pending_reg(uint16_t irq_id) { return &((uint16_t*)IRQ_PENDING_GRP0)[irq_group(irq_id)]; }
#define irq_handler(irqid) (a2560_irq_vectors[irq_group(irqid)][irq_number(irqid)])


/* Enable an interruption. First byte is group, second byte is bit */
void a2560u_irq_enable(uint16_t irq_id)
{
    a2560u_irq_acknowledge(irq_id);
    R16(irq_mask_reg(irq_id)) &= ~irq_mask(irq_id);
    a2560u_debugnl("a2560u_irq_enable(%02x) -> Mask %p=%04x", irq_id, irq_mask_reg(irq_id), R16(irq_mask_reg(irq_id)));
}


/* Disable an interruption. First byte is group, second byte is bit */
void a2560u_irq_disable(uint16_t irq_id)
{
    R16(irq_mask_reg(irq_id)) |= irq_mask(irq_id);
    a2560u_debugnl("a2560u_irq_disable(%02x) -> Mask %p=%04x", irq_id, irq_mask_reg(irq_id), R16(irq_mask_reg(irq_id)));
}


void a2560u_irq_acknowledge(uint8_t irq_id)
{
    R16(irq_pending_reg(irq_id)) = irq_mask(irq_id);
}


/* Set an interrupt handler for IRQ managed through GAVIN interrupt registers */
void *a2560u_irq_set_handler(uint16_t irq_id, void *handler)
{
    void *old_handler = irq_handler(irq_id);
    irq_handler(irq_id) = (void*)handler;

    a2560u_debugnl("a2560u_irq_set_handler(%04x,%p)", irq_id, handler);
    return old_handler;
}


/* Real Time Clock  **********************************************************/

void a2560u_clock_init(void)
{
    set_vector(INT_BQ4802LY_VECN, (uint32_t)a2560u_irq_bq4802ly);
    bq4802ly_init();
}


uint32_t a2560u_getdt(void)
{
    uint8_t day, month, hour, minute, second;
    uint16_t year;

    bq4802ly_get_datetime(&day, &month, &year, &hour, &minute, &second);
    a2560u_debugnl("RTC time= %02d/%02d/%04d %02d:%02d:%02d", day, month, year, hour, minute, second);

    return MAKE_ULONG(
        ((year-1980) << 9) | (month << 5) | day,
        (hour << 11) | (minute << 5) | (second>>1)/*seconds are of units of 2*/);
}


void a2560u_setdt(uint32_t datetime)
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
    a2560u_irq_disable(INT_KBD_PS2);
    a2560u_irq_disable(INT_MOUSE);

    /* Explain our setup to the PS/2 subsystem */
    ps2_config.port_data    = (uint8_t*)PS2_DATA;
    ps2_config.port_status  = (uint8_t*)PS2_CMD;
    ps2_config.port_cmd     = (uint8_t*)PS2_CMD;
    ps2_config.n_drivers    = sizeof(drivers)/sizeof(struct ps2_driver_t*);
    ps2_config.drivers      = drivers;

    ps2_init();

    /* Register GAVIN interrupt handlers */
    set_vector(INT_PS2KBD_VECN, (uint32_t)a2560u_irq_ps2kbd);
    set_vector(INT_PS2MOUSE_VECN, (uint32_t)a2560u_irq_ps2mouse);

    /* Acknowledge any pending interrupt */
    a2560u_irq_acknowledge(INT_KBD_PS2);
    a2560u_irq_acknowledge(INT_MOUSE);

    /* Go ! */
    a2560u_irq_enable(INT_KBD_PS2);
    a2560u_irq_enable(INT_MOUSE);
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


/* Utility functions *********************************************************/

static uint32_t set_vector(uint16_t num, uint32_t vector)
{
    uint32_t oldvector;
    uint32_t *addr = (uint32_t *) (4L * num);
    oldvector = *addr;

    if(vector != -1) {
        *addr = vector;
    }
    return oldvector;
}
