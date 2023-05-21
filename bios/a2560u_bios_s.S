// TOS Assembler routines for the Foenix Retro Systems A2560U

#include "config.h"
#ifdef MACHINE_A2560U

#include "../foenix/foenix.i"

| Exports ---------------------------------------------------------------------
    .GLOBAL a2560u_memconf
    .GLOBAL _a2560u_run_calibration
    .GLOBAL _a2560u_bios_irq_calibration


a2560u_memconf: // Called from memory.S to detect memory setup
    move.l  #0x200000,d5   // this is phystop - the end of RAM on A2560U. TODO: detect U+ (4MB)
    moveq   #0,d6          // fake memctrl
    jmp     (a6)           // Return


_a2560u_run_calibration: // Calibration loop
    // We should ideally be using a2560u_timer_enable in this function but we don't because we
    // don't want any overhead to get the most exact timings.
    move.l  d2,-(sp) // d2 is not a scratch register
    move.w  #~TIMER0_INT,IRQ_MASK_GRP1 // Mask everything except Timer 0
    move.l  _calibration_loop_count,d0

    // Enable timer 0 (cannot use RMW instruction)
    move.l  TIMER_CTRL0,d1
    move.l  d1,d2
    ori.l   #1,d1   // d1: start
    andi.l  #~1,d2  // d2: stop
    move.l  d1,TIMER_CTRL0
cal_loop:                           //just like asm.h inline function
    subq.l  #1,d0
    jpl     cal_loop

    // Disable timer 0, quick !
    move.l  d2,TIMER_CTRL0
    move.l  (sp)+,d2
    rts

_a2560u_bios_irq_calibration: // Timer 0 handler used for calibration loop, equivalent of handle_timer in delayasm.S
    move.w #TIMER0_INT,IRQ_PENDING_GRP1
    addq.l #1,_calibration_interrupt_count
    rte

#endif // MACHINE_A2560U