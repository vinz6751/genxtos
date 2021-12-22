| 
| VICKY -----------------------------------------------------------------------
.EQU VICKYII,               0xB40000 | VICKYII base address
.EQU VICKY,                 VICKYII
.EQU VICKY_CTRL,            VICKY

| Channel A (first screen)
.EQU VICKY_A_BORDER_CTRL_L, VICKY+0x0004 | Border control
.EQU VICKY_A_BORDER_COLOR,  VICKY+0x0008
.EQU VICKY_A_BG_COLOR,       VICKY+0x000C | Background control
.EQU VICKY_A_BMP_FG_CTRL,   VICKY+0x0100 | Bitmap layer 0 control
.EQU VICKY_A_BMP_FB,        VICKY+0x0104 | Framebuffer address relative to VRAM

.EQU VICKY_LUT,             VICKY+0x2000 | Color lookup tables, 0x400 bytes each

| Mouse
.EQU MOUSE_POINTER_MEMORY,  VICKY+0x0400
.EQU MOUSE_POINTER_CTRL,    VICKY+0x0C00

| Video memory sections
.EQU VRAM_Bank0,             0x00C00000 | 2MB (until 0xDFFFFF)


| GAVIN -----------------------------------------------------------------------
.EQU GAVIN,             0xB00000
    .EQU RTC,               (GAVIN+0x80)
        .EQU RTC_FLAGS,     (RTC+0x26)
| Interrupts
.EQU IRQ_PENDING_GRP0,  GAVIN+0x100
    .EQU VICKY_INT_VBL, 0x1
    .EQU VICKY_INT_HBL, 0x2
.EQU IRQ_PENDING_GRP1,  GAVIN+0x102
    .EQU PS2KEYB_INT,   0x0001
    .EQU PS2MOUSE_INT,  0x0004
    .EQU PS2MOUSE_INT,  0x0004
    .EQU COM1_INT,      0x0008
    .EQU RTC_INT,       0x8000
.EQU IRQ_PENDING_GRP2,  GAVIN+0x104
.EQU IRQ_POL_GRP0,      GAVIN+0x108
.EQU IRQ_POL_GRP1,      GAVIN+0x10A
.EQU IRQ_POL_GRP2,      GAVIN+0x10C
.EQU IRQ_EDGE_GRP0, 	GAVIN+0x110
.EQU IRQ_EDGE_GRP1, 	GAVIN+0x112
.EQU IRQ_EDGE_GRP2, 	GAVIN+0x114
.EQU IRQ_MASK_GRP0, 	GAVIN+0x118
.EQU IRQ_MASK_GRP1, 	GAVIN+0x11A
.EQU IRQ_MASK_GRP2, 	GAVIN+0x11C
.EQU UART0,             GAVIN+0x28F8

| BEATRIX ---------------------------------------------------------------------
.EQU BEATRIX,           0xB20000
.EQU PSG_PORT,          BEATRIX+0x130  | Control register for the SN76489
.EQU OPL3_PORT,         BEATRIX+0x200  | Access port for the OPL3
.EQU OPM_INT_BASE,      BEATRIX+0xC00  | Internal OPM base address
.EQU OPN2_INT_BASE,     BEATRIX+0xA00  | Internal OPN2 base address
.EQU CODEC,             BEATRIX+0xE00  | Control register for the CODEC




| Exports ---------------------------------------------------------------------
    .GLOBAL a2560u_memconf
    .GLOBAL _a2560u_irq_vicky
    .GLOBAL _a2560u_irq_bq4802ly
    .GLOBAL _a2560u_irq_ps2kbd
    .GLOBAL _a2560u_irq_ps2mouse
| Imports ---------------------------------------------------------------------
    .GLOBAL a2560_irq_vectors
    .GLOBAL _bq4802ly_ticks
    .GLOBAL _hz_200
    .GLOBAL _ps2_config
    .GLOBAL _ps2_channel1_irq_handler
    .GLOBAL _ps2_channel2_irq_handler
        
a2560u_memconf: // Called from memory.S to detect memory setup
    move.l  #0x200000,d5   // this is phystop - the end of RAM on A2560U. TODO: detect U+ (4MB)
    moveq   #0,d6          // fake memctrl
    jmp     (a6)           // Return


_a2560u_irq_vicky: // VICKY autovector interrupt handler
    // We use d3-d5/a3-a4 because a handler writtin in C function will preserve them
    // so we don't have to save/restore them before/after calling handlers.
    movem.l d3-d4/a3-a4,-(sp)
    lea     _a2560_irq_vectors,a3
    moveq   #1,d4               // We clear the MSB because that's how we exit the loop after 8 shifts
vicky_irq_loop:
    move.w  IRQ_PENDING_GRP0,d3 // is it ok to read in each iteration ?
    and.b   d4,d3           // the .b is deliberate, VICKY uses only the LSB the PENDING register
    beq.s   vicky_irq_next
    movea.l (a3)+,a4    // TODO on 68020+ use double indirection
#if 0 // See address of called handler
    movem.l d0-a6,-(sp)
    move.l  a4,-(sp)
    jsr     _debug1
    addq.l  #4,sp    
    movem.l (sp)+,d0-a6
#endif
    jsr     (a4)            // call handler
    move.w  d4,IRQ_PENDING_GRP0 // acknowledge
vicky_irq_next:    
    // Prepare next iteration
    lsl.b   #1,d4      // .b deliberate here as well
    bne.s   vicky_irq_loop
vicky_irq_done:
    movem.l (sp)+,d3-d4/a3-a4
    rte    


_a2560u_irq_bq4802ly: // Real time clock interrupt handler
    tst.b   RTC_FLAGS  // Acknowledge interrupt on the bq4802LY by reading the flags
    move.w  #RTC_INT,IRQ_PENDING_GRP1 // Acknowledge GAVIN interrupt
    addq.l  #1,_bq4802ly_ticks
    move.l  _bq4802ly_ticks,_hz_200 // We use the RTC timer as 200hz source. Not quite 200Hz since it's 3.90625ms rather than 5ms though.
    rte


_a2560u_irq_ps2kbd: // PS/2 keyboard interrupt handler
    move.w  #PS2KEYB_INT,IRQ_PENDING_GRP1
    movem.l d0-a6,-(sp) // TODO optimize
    jsr    _ps2_channel1_irq_handler
    movem.l (sp)+,d0-a6
    rte


_a2560u_irq_ps2mouse: // PS/2 mouse interrupt handler
    move.w  #PS2MOUSE_INT,IRQ_PENDING_GRP1
    movem.l d0-a6,-(sp) // TODO optimize
    jsr    _ps2_channel2_irq_handler
    movem.l (sp)+,d0-a6
    rte
