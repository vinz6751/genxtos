

#include <stdint.h>
#include <stdbool.h>
#include "foenix.h"
#include "a2560_debug.h"
#include "cpu.h"
#include "interrupts.h"
#include "regutils.h"
#include "timer.h"

extern uint32_t cpu_freq; /* CPU frequency */
void a2560_irq_calibration();

/* Timers ********************************************************************/

/* This holds all readily usable details of timers per timer number,
 * so we don't have to compute anything when setting a timer.
 * It's done so timers can be reprogrammed quickly without having to do
 * computation, so to get the best timing possible. */
/* Timers */
static const struct a2560_timer_t {
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
} a2560_timers[] =
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
    { TIMER_CTRL0, TIMER0_VALUE, TIMER0_COMPARE, 0xffffff00, TIMER_RESET << 0,  TIMER_PROG << 0,  TIMER_CTRL_ENABLE << 0,  1 << (INT_TIMER0&0xf), INT_TIMER0_VECN },
    { TIMER_CTRL0, TIMER1_VALUE, TIMER1_COMPARE, 0xffff00ff, TIMER_RESET << 8,  TIMER_PROG << 8,  TIMER_CTRL_ENABLE << 8,  1 << (INT_TIMER1&0xf), INT_TIMER1_VECN },
    { TIMER_CTRL0, TIMER2_VALUE, TIMER2_COMPARE, 0xff00ffff, TIMER_RESET << 16, TIMER_PROG << 16, TIMER_CTRL_ENABLE << 16, 1 << (INT_TIMER2&0xf), INT_TIMER2_VECN },
    { TIMER_CTRL1, TIMER3_VALUE, TIMER3_COMPARE, 0xffffff00, TIMER_RESET << 0,  TIMER_PROG << 0,  TIMER_CTRL_ENABLE << 0,  1 << (INT_TIMER3&0xf), INT_TIMER3_VECN }
};


/*
 * Initialise the timers.
 */
void a2560_timer_init(void)
{
    int i;

    /* Disable all timers */
    for (i = 0; i < sizeof(a2560_timers)/sizeof(struct a2560_timer_t); i++)
        a2560_timer_enable(i, false);
}

/*
 * Program a timer but don't start it. This causes the timer to stop.
 * timer: timer number
 * frequency: desired frequency
 * repeat: whether the timer should reload after firing interrupt
 * handler: routine to execute when timer fires
 */
void a2560_set_timer(uint16_t timer, uint32_t frequency, bool repeat, void *handler)
{
    struct a2560_timer_t *t;
    uint16_t sr;

    if (timer > 3)
        return;

    a2560_debugnl("Set timer %d, freq:%ldHz, repeat:%s, handler:%p",timer,frequency,repeat?"ON":"OFF",handler);
    
    /* Identify timer control register to use */
    t = (struct a2560_timer_t *)&a2560_timers[timer];

    /* We don't want interrupts while we reprogramming */
    sr = m68k_set_sr(0x2700);

    /* Stop the timer while we configure */
    a2560_timer_enable(timer, false);

    /* Stop and reprogram the timer, but don't start. */
    /* TODO: In case of control register 0, we write the config of 3 timers at once because
     * the timers control register has to be addressed as long word.
     * Can that have nasty effects on any other running timers ? */
    R32(t->control) = R32(t->control) & t->deprog;

    /* Set timer period */
#if TIMER_COUNT_UP
    R32(t->compare) = CPU_FREQ / frequency;
# define TIMER_USE_OFFICIAL_WAY 1
# if TIMER_USE_OFFICIAL_WAY
    R32(t->control) |= t->reset; /* Force value counter to 0 */
    R32(t->control) &= ~t->reset; /* Stop forcing it */
# else
    /* This works just as well and is a quicker */
    R32(t->value) = 0L;
# endif

#else /* TIMER_COUNT_DOWN */
    R32(t->value) = CPU_FREQ / frequency;
    R32(t->compare) = 0L;
#endif

    /* When counting up, the Value register is cleared whenever CLEAR **OR** RECLEAR is set.
     * Likewise when counting down, Value register is loaded whenever LOAD **OR** RELOAD is set. */

    R32(t->control) |= t->prog;

    /* Set handler */
    cpu_set_vector(t->vector, (uint32_t)handler);

    /* Before starting the timer, ignore any previous pending interrupt from it */
    if (R16(IRQ_PENDING_GRP1) & t->irq_mask)
        R16(IRQ_PENDING_GRP1) = t->irq_mask;

    /* Unmask interrupts for that timer */
    R16(IRQ_MASK_GRP1) &= ~t->irq_mask;

    m68k_set_sr(sr);
#if 0
    a2560_debugnl("AFTER SETTING TIMER %d", timer);
    a2560_debugnl("CPU freq     %ld", cpu_freq);
    a2560_debugnl("CPU          sr=%04x", get_sr());
    a2560_debugnl("vector       0x%02x=%p",t->vector,(void*)cpu_set_vector(t->vector, -1L));
    a2560_debugnl("value        %p=%p",(void*)t->value,R32(t->value));
    a2560_debugnl("compare      %p=%p",(void*)t->compare,R32(t->compare));
    a2560_debugnl("irq_pending  %p=%04x", (void*)IRQ_PENDING_GRP1,R16(IRQ_PENDING_GRP1));
    a2560_debugnl("irq_mask     %p=%04x", (void*)IRQ_MASK_GRP1,R16(IRQ_MASK_GRP1));
    a2560_debugnl("control      %p=%p",(void*)t->control,R32(t->control));
#endif
}

/*
 * Enable or disable a timer.
 * timer: timer number
 * enable: 0 to disable, anything to enable
 */
void a2560_timer_enable(uint16_t timer, bool enable)
{
    struct a2560_timer_t *t = (struct a2560_timer_t *)&a2560_timers[timer];
    a2560_debugnl("a2560_timer_enable(%d,%d)", timer, enable);
    if (enable)
        R32(t->control) |= t->start;
    else
        R32(t->control) &= ~t->start;

#if 0
    a2560_debugnl("After %s  > control %p=0x%08lx",enable?"Enable":"Disable", (void*)t->control,R32(t->control));
    if (R32(t->value) != R32(t->value))
        a2560_debugnl("Timer is running: 0x%08lx 0x%08lx 0x%08lx...",R32(t->value), R32(t->value), R32(t->value));
    else
        a2560_debugnl("Timer is not running");
#endif
}


/* Counts the number of iterations a tight loops the CPU can do in a given time.
 * This uses/clobbers timer 0. We run the test for a duration of calibration_time
 * (which is a rough number that is derived from the assumed CPU frequency so to make
 * sure we run the test "long enough", i.e. for a few milliseconds).
 * Returns the number of tight loops we ca do in the given calibration_time.
 */
uint32_t a2560_delay_calibrate(uint32_t calibration_time)
{
    uint16_t masks[IRQ_GROUPS];
    uint32_t old_timer_vector;
    uint32_t irq_count;

    a2560_debugnl("a2560_delay_calibrate(0x%ldms)", calibration_time);
    /* We should disable timer 0 now but we really don't expect that anything uses it during boot */

    /* Backup all interrupts masks because they would interfere with measuring time */
    a2560_irq_mask_all(masks);
    old_timer_vector = R32(INT_TIMER0_VECN*4);

    a2560_set_timer(0, 1000 /*1ms*/, true, a2560_irq_calibration);
    irq_count = a2560_run_calibration(calibration_time);

    /* Restore everything */
    R32(INT_TIMER0_VECN) = old_timer_vector;
    a2560_irq_restore(masks);

    a2560_debugnl("irq_count (old)= 0x%08lx, calibration_interrupt_count = %ld", calibration_time, irq_count);
    /* We have interrupts every 1ms so it's easy to count the number of tight loops per irq */
    if (irq_count)
        irq_count = calibration_time / irq_count;

    a2560_debugnl("irq_count (new)= 0x%08lx", irq_count);

    return irq_count;
}
