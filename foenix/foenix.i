// Assembler include file for Foenix hardware definitions

| VICKY -----------------------------------------------------------------------
.EQU VICKYII,               0xB40000 | VICKYII base address
.EQU VICKY,                 VICKYII
.EQU VICKY_CTRL,            VICKY
    .EQU VICKY_CTRL_DISABLE_RENDER, 0x80

| Channel A (first screen)
.EQU VICKY_A_BORDER_CTRL_L, VICKY+0x0004 | Border control
.EQU VICKY_A_BORDER_COLOR,  VICKY+0x0008
.EQU VICKY_A_BG_COLOR,      VICKY+0x000C | Background control
.EQU VICKY_A_BMP0_FG_CTRL,   VICKY+0x0100 | Bitmap layer 0 control
.EQU VICKY_A_BMP0_FB,        VICKY+0x0104 | Framebuffer address relative to VRAM

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
    .EQU COM1_INT,      0x0008
    .EQU TIMER0_INT,    0x0100
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
.EQU TIMER_CTRL0,       GAVIN+0x200
.EQU UART0,             GAVIN+0x28F8

.EQU UART16750_ISR,     2 | Interrupt status register
.EQU UART16750_LSR,     5 | Line status register

| BEATRIX ---------------------------------------------------------------------
.EQU BEATRIX,           0xB20000
.EQU PSG_PORT,          BEATRIX+0x130  | Control register for the SN76489
.EQU OPL3_PORT,         BEATRIX+0x200  | Access port for the OPL3
.EQU OPM_INT_BASE,      BEATRIX+0xC00  | Internal OPM base address
.EQU OPN2_INT_BASE,     BEATRIX+0xA00  | Internal OPN2 base address
.EQU CODEC,             BEATRIX+0xE00  | Control register for the CODEC

