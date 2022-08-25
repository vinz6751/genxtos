// Assembler routines for the Foenix Retro Systems A2560U

#include "config.h"
#ifdef MACHINE_A2560U
.EQU A2560U_DIRTY_CELLS_SIZE, 1024 // Needs to be the same as in a2560u.h.
                                   // The smaller this is, the higher MAX_CELLS_RENDERED_PER_FRAME needs to be.

.EQU DIRTY_CELLS_SIZE_SIZEOF, A2560U_DIRTY_CELLS_SIZE*4

#include "foenix.i"


| Exports ---------------------------------------------------------------------
    .GLOBAL a2560u_memconf
    .GLOBAL _a2560u_irq_vicky
    .GLOBAL _a2560u_irq_bq4802ly
    .GLOBAL _a2560u_irq_ps2kbd
    .GLOBAL _a2560u_irq_ps2mouse
    .GLOBAL _a2560u_irq_com1
    .GLOBAL _a2560u_run_calibration
    .GLOBAL _a2560u_irq_calibration
    .GLOBAL a2560u_copy_fb_to_vram
    .GLOBAL _a2560u_mark_cell_dirty
| Imports ---------------------------------------------------------------------
    .GLOBAL a2560_irq_vectors
    .GLOBAL _bq4802ly_ticks
    .GLOBAL _hz_200
    .GLOBAL _ps2_config
    .GLOBAL _ps2_channel1_irq_handler
    .GLOBAL _ps2_channel2_irq_handler
    .GLOBAL _calibration_loop_count
    .GLOBAL _calibration_interrupt_count
    .GLOBAL _a2560u_fb_size
    .GLOBAL _a2560u_vram_fb
    .GLOBAL _v_bas_ad
    .GLOBAL _push_serial_iorec
        
a2560u_memconf: // Called from memory.S to detect memory setup
    move.l  #0x200000,d5   // this is phystop - the end of RAM on A2560U. TODO: detect U+ (4MB)
    moveq   #0,d6          // fake memctrl
    jmp     (a6)           // Return


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
    move.l  _bq4802ly_ticks,_hz_200 // We use the RTC timer as 200hz source. Not quite 200Hz since it's 3.90625ms rather than 5ms though.
    rte


_a2560u_irq_ps2kbd: // PS/2 keyboard interrupt handler
    move.w  #PS2KEYB_INT,IRQ_PENDING_GRP1
    movem.l d0-d7/a0-a6,-(sp) // Save GCC scratch registers
    jbsr    _ps2_channel1_irq_handler
    movem.l (sp)+,d0-d7/a0-a6
    rte


_a2560u_irq_ps2mouse: // PS/2 mouse interrupt handler
    move.w  #PS2MOUSE_INT,IRQ_PENDING_GRP1
    movem.l d0-d7/a0-a6,-(sp) // Save GCC scratch registers
    jbsr    _ps2_channel2_irq_handler
    movem.l (sp)+,d0-d7/a0-a6
    rte


_a2560u_irq_com1: // UART COM1 interrupt handler
    // This is not completely working as it should because it seems the interrupts don't get acknowledged correctly
    // when we read the Interrupt Status Register (UART0+UART16750_ISR). As a result we can't acknowledge the
    // interrupts so we always get the highest priority one masking the others.
    // So instead of checking interrupts we examine the Line Status Register to find out the cause of the interrupt.
    movem.l d0-d1/a0-a2,-(sp)
com1_do:
    move.w  #COM1_INT,IRQ_PENDING_GRP1  // Acknowledge GAVIN interrupt
    move.b  UART0+UART16750_LSR,d1 // That "should" acknowledge the interrupt (but doesn't)
    btst    #0,d1
    beq.s   com1_done // No data received
    // Data received
    move.b  UART0,d0
    move.w  d0,-(sp)
    jbsr    _push_serial_iorec
    addq.l  #2,sp
    bra.s   com1_do
com1_done:
    movem.l (sp)+,d0-d1/a0-a2
    rte

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

_a2560u_irq_calibration: // Timer 0 handler used for calibration loop, equivalent of handle_timer in delayasm.S
    move.w #TIMER0_INT,IRQ_PENDING_GRP1
    addq.l #1,_calibration_interrupt_count
    rte


a2560u_copy_fb_to_vram: // Copies our SRAM buffer to video ram
    .GLOBAL _a2560u_dirty_cells
    .GLOBAL _v_lin_wr
    .GLOBAL _v_cel_ht

// Allow to limit the number of cells rendered per frame, to avoid spending too much time. -1 means unlimited.
// If it's not enough the dirty cells array may be too small and we loose data. If it's too high, we may spend
// too much time copying data and miss lower-priority interrupts (e.g. keyboard/mouse).
// Note: this doesn't apply to full screen refresh
#define MAX_CELLS_RENDERED_PER_FRAME 20

#if MAX_CELLS_RENDERED_PER_FRAME != -1
    move.l  #MAX_CELLS_RENDERED_PER_FRAME-1,d6 // max number of characters we render per frame
#endif

    lea     _a2560u_dirty_cells,a0
    tst.w   4(a0)           // Copy whole frame buffer to VRAM ?
    jne     copy_whole_screen  // Yes -> go there
    // Otherwise process the list of dirty cells from the ring buffer  
    move.w  2(a0),d0        // Get reader
    cmp.w   (a0),d0         // Compare with writer
    jeq     cell_copy_done  // Nothing to read -> end    
copy_cell_to_vram:
    // Cache information in registers
    movea.l _a2560u_vram_fb,a3
    move.l  _v_bas_ad.w,d2    // Cache
    //move.w  _v_cel_ht.w,d4    // Cache. d3 is the actual decounter used in the loop
    //subq.w  #1,d4           // For the dbra of cell height
    clr.l   d5
    move.w  _v_lin_wr,d5    // Cache for moving to next line of the frame buffer
cell_copy:
    addq.w  #4,d0           // Advance reader and wrap at 256 (offset 1024)
    andi.w  #DIRTY_CELLS_SIZE_SIZEOF-1,d0
    move.l  6(a0,d0),a1     // Get dirty cell address (in RAM frame buffer)
    movea.l a1,a2           // Compute target VRAM address
    suba.l  d2,a2           // d2: offset of dirty cell in frame buffer
    adda.l  a3,a2
    //move.w  d4,d3           // Prepare line decounter
copy_cell_to_fb:
    // Copy one row (8 bytes)
    // TODO copy 32bits at a time once possible (with current flash, it's not)
    .rept 16 // Font height 
    move.w  0(a1),0(a2)
    move.w  2(a1),2(a2)
    move.w  4(a1),4(a2)
    move.w  6(a1),6(a2)
    adda.l  d5,a1   // Next line
    adda.l  d5,a2
    .endr
    //dbra    d3,copy_cell_to_fb // If we're short on ROM space or the font is not 16pixcels high we can use dbra instead of .rept
#if MAX_CELLS_RENDERED_PER_FRAME != 1
    dbra    d6,cell_copy_more
    jbra    cell_copy_done
#endif
cell_copy_more:
    cmp.w   (a0),d0    // More to read ?
    jne     cell_copy  // If so, nothing more to read
cell_copy_done:
    move.w  d0,2(a0)   // Update reader
    rts    

copy_whole_screen:
    // TODO Use DMA once available
    // Copy the whole SRAM frame buffer to VRAM
    move.w  (a0),2(a0)          // Flush the ring buffer (reader <- writer)
    move.w  #0,4(a0)            // Put down the flag to force full copy, as we're doing it now
    move.l  _a2560u_fb_size,d0
    movea.l  _v_bas_ad,a1       // Source
    movea.l _a2560u_vram_fb,a3  // Destination  
    lsr.l   #5,d0               // We unroll the loop to copy 32 bytes at a time
fbcpy:
    movem.w (a1)+,d0-d7/a0/a2/a4-a6
    movem.w d0-d7/a0/a2/a4-a6,(a3)
    lea     13*2(a3),a3
    movem.w (a1)+,d0-d7/a0/a2/a4-a6
    movem.w d0-d7/a0/a2/a4-a6,(a3)
    lea     13*2(a3),a3
    movem.w (a1)+,d0-d7/a0/a2/a4-a6
    movem.w d0-d7/a0/a2/a4-a6,(a3)
    lea     13*2(a3),a3
    movem.w (a1)+,d0-d7/a0/a2/a4-a6
    movem.w d0-d7/a0/a2/a4-a6,(a3)
    lea     13*2(a3),a3
    movem.w (a1)+,d0-d7/a0/a2/a4-a5 // We do one less register here so an iteration is 128 bytes.
    movem.w d0-d7/a0/a2/a4-a5,(a3)  // As all resolutions have a screensize multiple of 128,
    lea     12*2(a3),a3             // That ensures we won't copy beyond the end of the FB in VRAM.
    cmpa.l  #0xc4b000,a3            // TODO BUG FIXME don't hardcode end of FB
    bmi.s   fbcpy
    rts


_a2560u_mark_cell_dirty:
    // Add the given cell to the ring buffer of dirty cells
    lea     _a2560u_dirty_cells,a0
    move.w  (a0),d0     // Writer
    addq.w  #4,d0       // Next position
    andi.w  #DIRTY_CELLS_SIZE_SIZEOF-1,d0   // Wrap if needed
    move.l  4(sp),6(a0,d0)
    move.w  d0,(a0)     // write writer
    rts

#endif // MACHINE_A2560U
