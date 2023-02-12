#include <stdbool.h>
#include <stdint.h>
#include "gavin_timer.h"
#include "foenix_cpu.h"
#include "../foenix.h"
#include "../regutils.h"


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
void gavin_timer_init(void)
{
    int i;

    /* Disable all timers */
    for (i = 0; i < sizeof(a2560u_timers)/sizeof(struct a2560u_timer_t); i++)
        gavin_timer_enable(i, false);
}

/*
 * Program a timer but don't start it. This causes the timer to stop.
 * timer: timer number
 * frequency: desired frequency
 * repeat: whether the timer should reload after firing interrupt
 * handler: routine to execute when timer fires
 */
void gavin_timer_set(uint16_t timer, uint32_t frequency, bool repeat, void *handler)
{
    struct a2560u_timer_t *t;
    uint16_t sr;

    if (timer > 3)
        return;

    //a2560u_debugnl("Set timer %d, freq:%ldHz, repeat:%s, handler:%p\n",timer,frequency,repeat?"ON":"OFF",handler);

    /* Identify timer control register to use */
    t = (struct a2560u_timer_t *)&a2560u_timers[timer];

    /* We don't want interrupts while we reprogramming */
    sr = cpu_disable_interrupts();

    /* Stop the timer while we configure */
    gavin_timer_enable(timer, false);

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
    cpu_set_vector(t->vector, handler);

    /* Before starting the timer, ignore any previous pending interrupt from it */
    if (R16(IRQ_PENDING_GRP1) & t->irq_mask)
        R16(IRQ_PENDING_GRP1) = t->irq_mask; /* Yes it's an assignment, no a OR. That's how interrupts are acknowledged */

    /* Unmask interrupts for that timer */
    R16(IRQ_MASK_GRP1) &= ~t->irq_mask;

    cpu_restore_interrupts(sr);
#if 0
    a2560u_debugnl("AFTER SETTING TIMER %d\n", timer);
    a2560u_debugnl("CPU          sr=%04x\n", get_sr());
    a2560u_debugnl("vector       0x%02x=%p\n",t->vector,(void*)cpu_set_vector(t->vector, -1L));
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
void gavin_timer_enable(uint16_t timer, bool enable)
{
    struct a2560u_timer_t *t = (struct a2560u_timer_t *)&a2560u_timers[timer];

    if (enable)
        R32(t->control) |= t->start;
    else
        R32(t->control) &= ~t->start;
}
