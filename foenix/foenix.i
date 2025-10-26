// Assembler include file for Foenix hardware definitions

#ifdef MACHINE_A2560U
#define CONF_WITH_MPU401 0
.EQU SRAM_TOP,          0x400000
.EQU GAVIN,             0xB00000
.EQU BEATRIX,           0xB20000
.EQU VICKYII,           0xB40000 | VICKYII base address
.EQU VRAM_Bank0,        0xC00000 | 2MB (until 0xDFFFFF)
#elif defined (MACHINE_A2560X) || defined(MACHINE_GENX) || defined(MACHINE_A2560M) || defined(MACHINE_A2560K)
#define CONF_WITH_MPU401 1
.EQU SRAM_TOP,          0x00400000
.EQU SDRAM,             0x02000000 | 64MB (until 0x04FFFFFF)
.EQU GAVIN,             0xFEC00000
.EQU SUPERIO_BASE,      0xFEC02000
.EQU BEATRIX,           0xFEC20000
.EQU FLASH0,            0xFFC00000
#endif

#if defined(MACHINE_A2560K) || defined (MACHINE_A2560X) || defined(MACHINE_GENX)
// I have deliberately inverted bank 0 and 1 so that bank 0 is, for both the U and the X/K/GenX the "full featured" bank
.EQU VRAM_Bank1,        0x00800000 | 4MB (until 0xBFFFFF)
.EQU VRAM_Bank0,        0x00C00000 | 4MB (until 0xDFFFFF)
.EQU VICKY_A,           0xFEC40000
.EQU VICKY_B,           0xFEC80000
#elif defined(MACHINE_A2560M)
.EQU VICKY3,            0xFC000000 | VICKYIII base address
#endif

| VICKY -----------------------------------------------------------------------
.EQU VICKY,                 VICKY_B
.EQU VICKY_CTRL,            VICKY
    .EQU VICKY_CTRL_DISABLE_RENDER, 0x80

| Channel A (first screen)
.EQU VICKY_BORDER_CTRL_L,   VICKY+0x0004 | Border control
.EQU VICKY_B_BORDER_COLOR,  VICKY_B+0x0008
.EQU VICKY_B_BG_COLOR,      VICKY_B+0x000C | Background control
.EQU VICKY_A_BMP0_FG_CTRL,  VICKY_B+0x0100 | Bitmap layer 0 control
.EQU VICKY_A_BMP0_FB,       VICKY_B+0x0104 | Framebuffer address relative to VRAM

.EQU VICKY_LUT,             VICKY_B+0x2000 | Color lookup tables, 0x400 bytes each

| Mouse
.EQU MOUSE_POINTER_MEMORY,  VICKY_B+0x0400
.EQU MOUSE_POINTER_CTRL,    VICKY_B+0x0C00

| Video memory sections



| GAVIN -----------------------------------------------------------------------
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
    .EQU COM2_INT,      0x0010
    .EQU LPT_INT,       0x0020
    .EQU FDC_INT,       0x0040
    .EQU MIDI_INT,      0x0080
    .EQU TIMER0_INT,    0x0100
    .EQU TIMER1_INT,    0x0200
    .EQU TIMER2_INT,    0x0400
    .EQU TIMER3_INT,    0x0800
    .EQU TIMER4_INT,    0x1000
    .EQU LAB_INT,       0x4000
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
.EQU PSG_PORT,          BEATRIX+0x130  | Control register for the SN76489
.EQU OPL3_PORT,         BEATRIX+0x200  | Access port for the OPL3
.EQU OPM_INT_BASE,      BEATRIX+0xC00  | Internal OPM base address
.EQU OPN2_INT_BASE,     BEATRIX+0xA00  | Internal OPN2 base address
.EQU CODEC,             BEATRIX+0xE00  | Control register for the CODEC

