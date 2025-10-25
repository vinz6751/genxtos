| Exports ---------------------------------------------------------------------
    .GLOBAL _a2560_run_calibration
    .GLOBAL _a2560_irq_calibration

#include "foenix.h"

#if defined(MACHINE_A2560M)
#define TIMER0_INT 0x0200
#else
#define TIMER0_INT 0x0100
#endif

_a2560_run_calibration: // Calibration loop
    move.l  4(sp),d0 // Number of iterations before we stop the timer

    // We should ideally be using a2560_timer_enable in this function but we don't because we
    // don't want any overhead to get the most exact timings.
    movem.l  d2-d4/a3,-(sp)
    move.w  #~TIMER0_INT,IRQ_MASK_GRP1 // Mask everything except Timer 0

    // Preload registers for the IRQ handler so it runs as fast as possible
    move.w  #TIMER0_INT,d3
    lea     IRQ_PENDING_GRP1,a3

    clr.l   d4 // IRQ counter

    // Enable timer 0 (cannot use RMW instruction)
    move.l  TIMER_CTRL0,d1
    move.l  d1,d2
    ori.l   #1,d1   // d1: start
    andi.l  #~1,d2  // d2: stop
    move.l  d1,TIMER_CTRL0
    
cal_loop: // Replicate asm.h's delay_loop() function
    subq.l  #1,d0
    jpl     cal_loop

    // Disable timer 0, quick !
    move.l  d2,TIMER_CTRL0
    
    move.l  d4,d0 // return value
    movem.l (sp)+,d2-d4/a3
    rts


_a2560_irq_calibration: // Timer 0 handler used for calibration loop, equivalent of handle_timer in delayasm.S
    move.w d3,(a3)  // Acknowledge interrupt
    addq.l #1,d4    // Increase interruption counter
    rte