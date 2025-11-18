// Assembler routines for the Foenix Retro Systems A2560U

// Note: gcc 4.6.4 uses d0-d1/a0-a1 as scratch registers. TOS also uses d2/a2
// so if somebody hooks in, they may expect these to be saved too. If we call
// TOS stuff from here that can be hooked, we need to save d2 as well.

#include "foenix.h"

| Exports ---------------------------------------------------------------------
#if defined(MACHINE_A2560K) || defined(MACHINE_A2560X) || defined(MACHINE_GENX)
    .GLOBAL _a2560_irq_vicky_a
    .GLOBAL _a2560_irq_vicky_b
#elif defined(MACHINE_A2560U) || defined(MACHINE_A2560M)
    .GLOBAL _a2560_irq_vicky
#else
 #error "Need to specify Foenix model"
#endif
    .GLOBAL _a2560_irq_bq4802ly
    .GLOBAL _a2560_irq_ps2kbd
    .GLOBAL _a2560_irq_ps2mouse
    .GLOBAL _a2560_irq_com1
#if defined(MACHINE_A2560K) || defined(MACHINE_A2560M) || defined(MACHINE_A2560X) || defined(MACHINE_GENX)
    .GLOBAL _a2560_irq_com2
    .GLOBAL _a2560_irq_mpu401
#endif
    .GLOBAL _a2560_rte
    .GLOBAL _a2560_rts

| Imports ---------------------------------------------------------------------
    .GLOBAL a2560_irq_vectors
    .GLOBAL _bq4802ly_ticks
    .GLOBAL _ps2_channel1_irq_handler
    .GLOBAL _ps2_channel2_irq_handler
    .GLOBAL _calibration_loop_count
    .GLOBAL _calibration_interrupt_count
    .GLOBAL _uart16550_rx_handler
    .GLOBAL _bq4802ly_tick_handler
#if CONF_WITH_MPU401
    .GLOBAL _mpu401_rx_handler
#endif

_a2560_rts:
    rts


_a2560_rte:
    rte


#if defined(MACHINE_A2560K) || defined(MACHINE_A2560X) || defined(MACHINE_GENX)
_a2560_irq_vicky_a: // VICKY autovector A interrupt handler
#elif defined(MACHINE_A2560U) || defined(MACHINE_A2560M)
_a2560_irq_vicky: // VICKY (only channel's) autovector interrupt handler
#endif
    // Handlers can use d0 and a0 without saving them.
    movem.l d0-d1/a0-a1,-(sp)
    lea     _a2560_irq_vectors,a0
    moveq   #1,d0               // We must clear the MSB of the LSW because that's how we exit the loop after 7 shifts
vicky_irq_loop:
    move.w  IRQ_PENDING_GRP0,d1 // Is it ok to read in each iteration ? if not we'd have to save d1
    and.b   d0,d1               // The .b is deliberate, VICKY uses only the LSB of the PENDING register
    beq.s   vicky_irq_next
    move.w  d0,IRQ_PENDING_GRP0 // Acknowledge
    movea.l (a0)+,a1            // TODO on 68020+ use double indirection
    movem.l d0/a0,-(sp)
    jsr     (a1)                // Call handler
    movem.l (sp)+,d0/a0
vicky_irq_next:
    // Prepare next iteration
    lsl.b   #1,d0               // .b deliberate here as well
#if defined(MACHINE_A2560M)
    // A2560M only supports VBL/HBL for now (2 other bits I'm not sure if it's an error ?)
    cmp.b #0b10000,d0
#endif
    bne.s   vicky_irq_loop
vicky_irq_done:
    movem.l (sp)+,d0-d1/a0-a1
    rte


#if defined(MACHINE_A2560K) || defined(MACHINE_A2560M) || defined(MACHINE_A2560X) || defined(MACHINE_GENX)
_a2560_irq_vicky_b: // VICKY channel B autovector interrupt handler
    // Handlers can use d0 and a0 without saving them.
    movem.l d0-d1/a0-a1,-(sp)
    lea     _a2560_irq_vectors,a0
    move.w   #0x0100,d0         // We must clear the MSb of the MSB because that's how we exit the loop after 7 shifts
vicky_irq_b_loop:
    move.w  IRQ_PENDING_GRP0,d1 // Is it ok to read in each iteration ? if not we'd have to save d1
    and.w   d0,d1
    beq.s   vicky_irq_b_next
    move.w  d0,IRQ_PENDING_GRP0 // Acknowledge
    movea.l (a0)+,a1            // TODO on 68020+ use double indirection
    movem.l d0/a0,-(sp)
    jsr     (a1)                // Call handler
    movem.l (sp)+,d0/a0
vicky_irq_b_next:
    // Prepare next iteration
    lsl.w   #1,d0               // .w deliberate here so we stop on the last bit of the interrupt group
    bne.s   vicky_irq_b_loop
vicky_irq_b_done:
    movem.l (sp)+,d0-d1/a0-a1
    rte
#endif


_a2560_irq_bq4802ly: // Real time clock interrupt handler
    tst.b   BQ4802LY_BASE+0x26  // Acknowledge interrupt on the bq4802LY by reading the flags
    move.w  #(1<<INT_BIT(INT_RTC)),INT_GRP(INT_RTC) // Acknowledge GAVIN interrupt
    addq.l  #1,_bq4802ly_ticks
    move.l  _bq4802ly_tick_handler,-(sp)
    rts     // The handler must save register and terminate with rte


_a2560_irq_ps2kbd: // PS/2 keyboard interrupt handler
    // This interrupt has low priority and if its processing is delayed by another interrupt with
    // higher priority, we could be loosing data due to overruns. So when this handler is called,
    // we don't want it to be interrupted.
    move.w  #0x2700,sr
    move.w  #(1<<INT_BIT(INT_KBD_PS2)),INT_GRP(INT_KBD_PS2)
    movem.l d0-d2/a0-a2,-(sp) // Save GCC scratch registers
    jbsr    _ps2_channel1_irq_handler
    movem.l (sp)+,d0-d2/a0-a2
    rte


_a2560_irq_ps2mouse: // PS/2 mouse interrupt handler
    // This interrupt has low priority and if its processing is delayed by another interrupt with
    // higher priority, we could be loosing data due to overruns. So when this handler is caller,
    // we don't want it to be interrupted.
    move.w  #0x2700,sr
    move.w  #(1<<INT_BIT(INT_MOUSE)),INT_GRP(INT_MOUSE)
    movem.l d0-d2/a0-a2,-(sp) // Save GCC scratch registers
    jbsr    _ps2_channel2_irq_handler
    movem.l (sp)+,d0-d2/a0-a2
    rte


_a2560_irq_com1: // UART COM1 interrupt handler
    // On the A2560U this is not completely working as it should because it seems the interrupts don't get acknowledged correctly
    // when we read the Interrupt Status Register (UART0+UART16750_ISR). As a result we can't acknowledge the
    // interrupts so we always get the highest priority one masking the others.
    // So instead of checking interrupts we examine the Line Status Register to find out the cause of the interrupt.
    movem.l d0-d2/a0-a2,-(sp)
com1_do:
    move.w  #(1<<INT_BIT(INT_COM1)),INT_GRP(INT_COM1)
    lea     UART1,a0
    move.b  5(a0),d1 // 5 is the Line Status Register. Reading "should" acknowledge the interrupt (but doesn't on the U)
    btst    #0,d1    // Receive register full ?
    beq.s   com_done // No data received
    // Data received
    move.b  0(a0),d0
    move.w  d0,-(sp)
    movea.l _uart16550_rx_handler,a0
    jbsr    (a0)
    addq.l  #2,sp
    bra.s   com1_do
com_done:
    movem.l (sp)+,d0-d2/a0-a2
    rte

#if defined(MACHINE_A2560K) || defined(MACHINE_A2560M) || defined(MACHINE_A2560X) || defined(MACHINE_GENX)

/* That must mean we have a SuperIO therefore a 2nd serial port */
_a2560_irq_com2: // UART COM2 interrupt handler
    movem.l d0-d2/a0-a2,-(sp)
com2_do:
    move.w  #(1<<INT_BIT(INT_COM2)),INT_GRP(INT_COM2)
    lea     UART2,a0
    move.b  5(a0),d1 // 5 is the Line Status Register
    btst    #0,d1
    beq.s   com_done // No data received
    // Data received
    move.b  0(a0),d0
    move.w  d0,-(sp)
    movea.l _uart16550_rx_handler,a0
    jbsr    (a0)
    addq.l  #2,sp
    bra.s   com2_do


_a2560_irq_mpu401:
    | Handle MIDI interrupts. Note that there is no overrun or frame error
    | detection supported by the SuperIO's MPU-401 implementation (contrary to the
    | Atari ST's 6850 ACIA)
    move.w  #0x2700,sr
    move.w  #(1<<INT_BIT(INT_MIDI)),INT_GRP(INT_MIDI)
    movem.l d0-d2/a0-a2,-(sp)   // Save GCC scratch registers
    move.b  MPU401_BASE,d0
    move.b  d0,-(sp) // we pass the byte both in d0 (convenience for EmuTOS's midivec) and on the stack
    move.l  _mpu401_rx_handler,a0
    addq.l  #2,sp // sp is always even
    jbsr    (a0)
    movem.l (sp)+,d0-d2/a0-a2
    rte
#endif
