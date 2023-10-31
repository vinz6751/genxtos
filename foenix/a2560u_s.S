// Assembler routines for the Foenix Retro Systems A2560U

// Note: gcc 4.6.4 uses d0-d1/a0-a1 as scratch registers. TOS also uses d2/a2
// so if somebody hooks in, they may expect these to be saved too. If we call
// TOS stuff from here that can be hooked, we need to save d2 as well.

#include "foenix.i"


| Exports ---------------------------------------------------------------------
    .GLOBAL _a2560u_irq_vicky
    .GLOBAL _a2560u_irq_bq4802ly
    .GLOBAL _a2560u_irq_ps2kbd
    .GLOBAL _a2560u_irq_ps2mouse
    .GLOBAL _a2560u_irq_com1
    .GLOBAL _a2560u_rte
    .GLOBAL _a2560u_rts

| Imports ---------------------------------------------------------------------
    .GLOBAL a2560_irq_vectors
    .GLOBAL _bq4802ly_ticks
    .GLOBAL _hz_200
    .GLOBAL _ps2_config
    .GLOBAL _ps2_channel1_irq_handler
    .GLOBAL _ps2_channel2_irq_handler
    .GLOBAL _calibration_loop_count
    .GLOBAL _calibration_interrupt_count
    .GLOBAL _uart16550_rx_handler
    .GLOBAL _bq4802ly_tick_handler


_a2560u_rts:
    rts


_a2560u_rte:
    rte


_a2560u_irq_vicky: // VICKY autovector interrupt handler
    // Handlers can use d0 and a0 without saving them.
    movem.l d0-d1/a0-a1,-(sp)
    lea     _a2560_irq_vectors,a0
    moveq   #1,d0               // We must clear the MSB of the LSW because that's how we exit the loop after 8 shifts
vicky_irq_loop:
    move.w  IRQ_PENDING_GRP0,d1 // Is it ok to read in each iteration ? if not we'd have to save d1
    and.b   d0,d1               // The .b is deliberate, VICKY uses only the LSB the PENDING register
    beq.s   vicky_irq_next
    move.w  d0,IRQ_PENDING_GRP0 // Acknowledge
    movea.l (a0)+,a1            // TODO on 68020+ use double indirection
    movem.l d0/a0,-(sp)
    jsr     (a1)                // Call handler
    movem.l (sp)+,d0/a0
vicky_irq_next:    
    // Prepare next iteration
    lsl.b   #1,d0               // .b deliberate here as well
    bne.s   vicky_irq_loop
vicky_irq_done:
    movem.l (sp)+,d0-d1/a0-a1
    rte


_a2560u_irq_bq4802ly: // Real time clock interrupt handler
    tst.b   RTC_FLAGS  // Acknowledge interrupt on the bq4802LY by reading the flags
    move.w  #RTC_INT,IRQ_PENDING_GRP1 // Acknowledge GAVIN interrupt
    addq.l  #1,_bq4802ly_ticks
    move.l  _bq4802ly_tick_handler,-(sp)
    rts     // The handler must save register and terminate with rte


_a2560u_irq_ps2kbd: // PS/2 keyboard interrupt handler
    // This interrupt has low priority and if its processing is delayed by another interrupt with
    // higher priority, we could be loosing data due to overruns. So when this handler is caller,
    // we don't want it to be interrupted.
    move.w  sr,-(sp)
    move.w  #0x2700,sr
    move.w  #PS2KEYB_INT,IRQ_PENDING_GRP1
    movem.l d0-d2/a0-a2,-(sp) // Save GCC scratch registers
    jbsr    _ps2_channel1_irq_handler
    movem.l (sp)+,d0-d2/a0-a2
    move.w  (sp)+,sr
    rte


_a2560u_irq_ps2mouse: // PS/2 mouse interrupt handler
    // This interrupt has low priority and if its processing is delayed by another interrupt with
    // higher priority, we could be loosing data due to overruns. So when this handler is caller,
    // we don't want it to be interrupted.
    move.w  sr,-(sp)
    move.w  #0x2700,sr
    move.w  #PS2MOUSE_INT,IRQ_PENDING_GRP1
    movem.l d0-d2/a0-a2,-(sp) // Save GCC scratch registers
    jbsr    _ps2_channel2_irq_handler
    movem.l (sp)+,d0-d2/a0-a2
    move.w  (sp)+,sr
    rte


_a2560u_irq_com1: // UART COM1 interrupt handler
    // This is not completely working as it should because it seems the interrupts don't get acknowledged correctly
    // when we read the Interrupt Status Register (UART0+UART16750_ISR). As a result we can't acknowledge the
    // interrupts so we always get the highest priority one masking the others.
    // So instead of checking interrupts we examine the Line Status Register to find out the cause of the interrupt.
    movem.l d0-d1/a0-a2,-(sp)
com1_do:
    move.w  #COM1_INT,IRQ_PENDING_GRP1  // Acknowledge GAVIN interrupt (if any)
    move.b  UART0+UART16750_LSR,d1 // That "should" acknowledge the interrupt (but doesn't)
    btst    #0,d1
    beq.s   com1_done // No data received
    // Data received
    move.b  UART0,d0
    move.w  d0,-(sp)
    movea.l _uart16550_rx_handler,a0
    jbsr    (a0)
    addq.l  #2,sp
    bra.s   com1_do
com1_done:
    movem.l (sp)+,d0-d1/a0-a2
    rte
