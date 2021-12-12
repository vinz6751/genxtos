/*
 * foenix.h - Foenix Retro System computer specific defines
 *
 * Copyright (C) 2013-2021 The EmuTOS development team
 *
 * Authors:
 *  VRI   Vincent Barrilliot
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#ifndef FOENIX_H
#define FOENIX_H

#include <stdint.h>

#ifndef R8
    #define R8(x) *((volatile int8_t * const)(x))
#endif
#ifndef R16
    #define R16(x) *((volatile uint16_t * const)(x))
#endif
#ifndef R32
    #define R32(x) *((volatile uint32_t * const)(x))
#endif


#ifdef MACHINE_A2560U

#define CPU_FREQ    20000000 /* 20Mhz : TODO get rid of this and use cpu_freq */

#define GAVIN       0x00B00000
#define BEATRIX     0x00B20000
#define VICKY       0x00B40000
#define VRAM_Bank0  0x00C00000 /* 2MB (until 0xDFFFFF) */

/* Serial port speed codes for a2560u_serial_set_bps */
#define UART0       (UART16550*)(GAVIN+0x28F8)
#define UART_300    4167 /* Code for 300 bps */
#define UART_1200   1042 /* Code for 1200 bps */
#define UART_2400   521  /* Code for 2400 bps */
#define UART_4800   260  /* Code for 4800 bps */
#define UART_9600   130  /* Code for 9600 bps */
#define UART_19200  65   /* Code for 19200 bps */
#define UART_38400  33   /* Code for 28400 bps */
#define UART_57600  22   /* Code for 57600 bps */
#define UART_115200 11   /* Code for 115200 bps */

/* Interrupts */
#define IRQ_GROUPS        3 /* Number of IRQ groups */
#define IRQ_PENDING_GRP0 (GAVIN+0x100)
#define IRQ_PENDING_GRP1 (GAVIN+0x102)
#define IRQ_PENDING_GRP2 (GAVIN+0x104)
#define IRQ_POL_GRP0     (GAVIN+0x108)
#define IRQ_POL_GRP1     (GAVIN+0x10A)
#define IRQ_POL_GRP2     (GAVIN+0x10C)
#define IRQ_EDGE_GRP0 	 (GAVIN+0x110)
#define IRQ_EDGE_GRP1 	 (GAVIN+0x112)
#define IRQ_EDGE_GRP2 	 (GAVIN+0x114)
#define IRQ_MASK_GRP0 	 (GAVIN+0x118)
#define IRQ_MASK_GRP1 	 (GAVIN+0x11A)
#define IRQ_MASK_GRP2 	 (GAVIN+0x11C)

/* Interrupt vector numbers (not address!)  */
#define INT_TIMERS_VECN  0x48
#define INT_TIMER0_VECN  (INT_TIMERS_VECN+0) /* Timer 0, Clocked with the CPU Clock */
#define INT_TIMER1_VECN  (INT_TIMERS_VECN+1) /* Timer 1, Clocked with the CPU Clock */
#define INT_TIMER2_VECN  (INT_TIMERS_VECN+2) /* Timer 2, Clocked with the CPU Clock */
#define INT_TIMER3_VECN  (INT_TIMERS_VECN+3) /* Timer 3, Clocked with the SOF Channel A */
#define INT_NIRQ         4

#if 0
/*
 * Define standard interrupt numbers to be used for enabling, disabling an interrupt or setting its handler
 */

#define INT_SOF_A           0x00    /* Vicky Channel A Start of Frame */
#define INT_SOL_A           0x01    /* Vicky Channel A Start of Line */
#define INT_VICKY_A_1       0x02
#define INT_VICKY_A_2       0x03
#define INT_VICKY_A_3       0x04
#define INT_VICKY_A_4       0x05
#define INT_RESERVED_1      0x06
#define INT_VICKY_A_DAC     0x07
#define INT_SOF_B           0x08    /* Vicky Channel B Start of Frame */
#define INT_SOL_B           0x09    /* Vicky Channel B Start of Line */
#define INT_VICKY_B_1       0x0A
#define INT_VICKY_B_2       0x0B
#define INT_VICKY_B_3       0x0C
#define INT_VICKY_B_4       0x0D
#define INT_RESERVED_2      0x0E
#define INT_VICKY_B_DAC     0x0F

#define INT_KBD_PS2         0x10    /* SuperIO - PS/2 Keyboard */
#define INT_KBD_A2560K      0x11    /* SuperIO - A2560K Built in keyboard (Mo) */
#define INT_MOUSE           0x12    /* SuperIO - PS/2 Mouse */
#define INT_COM1            0x13    /* SuperIO - COM1 */
#define INT_COM2            0x14    /* SuperIO - COM2 */
#define INT_LPT1            0x15    /* SuperIO - LPT1 */
#define INT_FDC             0x16    /* SuperIO - Floppy Drive Controller */
#define INT_MIDI            0x17    /* SuperIO - MIDI */
#define INT_TIMER0          0x18    /* Timer 0, Clocked with the CPU Clock */
#define INT_TIMER1          0x19    /* Timer 1, Clocked with the CPU Clock */
#define INT_TIMER2          0x1A    /* Timer 2, Clocked with the CPU Clock */
#define INT_TIMER3          0x1B    /* Timer 3, Clocked with the SOF Channel A */
#define INT_TIMER4          0x1C    /* Timer 4, Clocked with the SOF Channel B */
#define INT_RESERVED_3      0x1D    /* Reserved */
#define INT_RESERVED_4      0x1E    /* Reserved */
#define INT_RTC             0x1F    /* Real Time Clock */

#define INT_PATA            0x20    /* IDE/PATA Hard drive interrupt */
#define INT_SDC_INS         0x21    /* SD card inserted */
#define INT_SDC             0x22    /* SD card controller */
#define INT_OPM_INT         0x23    /* Internal OPM */
#define INT_OPN2_EXT        0x24    /* External OPN */
#define INT_OPL3_EXT        0x25    /* External OPL */
#define INT_RESERVED_5      0x26    /* Reserved */
#define INT_RESERVED_6      0x27    /* Reserved */
#define INT_BEATRIX_0       0x28    /* Beatrix 0 */
#define INT_BEATRIX_1       0x29    /* Beatrix 1 */
#define INT_BEATRIX_2       0x2A    /* Beatrix 2 */
#define INT_BEATRIX_3       0x2B    /* Beatrix 3 */
#define INT_RESERVED_7      0x2C    /* Reserved */
#define INT_DAC1_PB         0x2D    /* DAC1 Playback Done (48K) */
#define INT_RESERVED_8      0x2E    /* Reserved */
#define INT_DAC0_PB         0x2F    /* DAC0 Playback Done (44.1K) */
#endif

/* Timers */
#define TIMER_CTRL0    (GAVIN+0x200)
#define TIMER_CTRL1    (GAVIN+0x204)
#define TIMER0_VALUE   (GAVIN+0x208)
#define TIMER0_COMPARE (GAVIN+0x20C)
#define TIMER1_VALUE   (GAVIN+0x210)
#define TIMER1_COMPARE (GAVIN+0x214)
#define TIMER2_VALUE   (GAVIN+0x218)
#define TIMER2_COMPARE (GAVIN+0x21C)
#define TIMER3_VALUE   (GAVIN+0x220)
#define TIMER3_COMPARE (GAVIN+0x224)
/* Flags for timer control */
#define TIMER_CTRL_ENABLE   0x01
#define TIMER_CTRL_CLEAR    0x02 /* Set this if counting up */
#define TIMER_CTRL_LOAD     0x04 /* Set this if counting down */
#define TIMER_CTRL_UPDOWN   0x08 /* Set this if counting down */
#define TIMER_CTRL_RECLEAR  0x10 /* Set this to auto recommence counting up */
#define TIMER_CTRL_RELOAD   0x20 /* Set this if auto recommence counting down */

/* BEATRIX */
#define SN76489_PORT  ((volatile uint8_t*)(BEATRIX+0x0130))   /* Control register for the SN76489 */
#define OPL3_PORT     ((volatile uint8_t*)(BEATRIX+0x0200))   /* Access port for the OPL3 */
#define OPM_INT_BASE  ((volatile uint8_t*)(BEATRIX+0x0C00))   /* Internal OPM base address */
#define OPN2_INT_BASE ((volatile uint8_t*)(BEATRIX+0x0A00))   /* Internal OPN2 base address */
#define WM8776_PORT   ((uint16_t*)(BEATRIX+0x0E00))         /* Mixer/codec port */

#endif


#ifdef MACHINE_C256FOENIXGENX


// ----------------- PS/2 ------------------------
// TODO
#define PS2_KDATA 0x60
#define PS2_KCTRL 0x64

// Bits of the PS/2 status register
#define PS2_STATUS_OUPUT    (1 << 0) // Output buffer status. 0:empty, 1: full
#define PS2_STATUS_INPUT    (1 << 1) // Input buffer status. 0:empty, 1: full
#define PS2_STATUS_SYSTEM   (1 << 2) // System flag.Clared on reset and set by firmware when POST is successful
#define PS2_STATUS_CTRLDATA (1 << 3) // Command/data (0 = data written to input buffer is data for PS/2 device, 1 = data written to input buffer is data for PS/2 controller command)
#define PS2_STATUS_TIMEO    (1 << 6) // Time out error flag
#define PS2_STATUS_PARITY   (1 << 7) // Parity error flag


// GenX memory map
// ----------------- INTERRUPTS ------------------
// Register Block 1
#define FNX1_INT00_KBD        0x01  // Keyboard Interrupt
#define FNX1_INT01_COL0       0x02  // VICKY_II (INT2) Sprite Collision 
#define FNX1_INT02_COL1       0x04  // VICKY_II (INT3) Bitmap Collision
#define FNX1_INT03_COM2       0x08  // Serial Port 2
#define FNX1_INT04_COM1       0x10  // Serial Port 1
#define FNX1_INT05_MPU401     0x20  // Midi Controller Interrupt
#define FNX1_INT06_LPT        0x40  // Parallel Port
#define FNX1_INT07_SDCARD     0x80  // SD Card Controller Interrupt (CH376S)
// Register Block 2			// 
#define FNX2_INT00_OPL3       0x01  // OPl3
#define FNX2_INT01_GABE_INT0  0x02  // GABE (INT0) - TBD
#define FNX2_INT02_GABE_INT1  0x04  // GABE (INT1) - TBD
#define FNX2_INT03_VDMA       0x08  // VICKY_II (INT4) - VDMA Interrupt
#define FNX2_INT04_COL2       0x10  // VICKY_II (INT5) Tile Collision
#define FNX2_INT05_GABE_INT2  0x20  // GABE (INT2) - TBD
#define FNX2_INT06_EXT        0x40  // External Expansion
#define FNX2_INT07_SDCARD_INS 0x80  //  SDCARD Insertion
// Register Block 3 (FMX Expansion)
#define FNX3_INT00_OPN2       0x01  // OPN2
#define FNX3_INT01_OPM        0x02  // OPM
#define FNX3_INT02_IDE        0x04  // HDD IDE INTERRUPT
#define FNX3_INT03_TBD        0x08  // TBD
#define FNX3_INT04_TBD        0x10  // TBD
#define FNX3_INT05_TBD        0x20  // GABE (INT2) - TBD
#define FNX3_INT06_TBD        0x40  // External Expansion
#define FNX3_INT07_TBD        0x80  //  SDCARD Insertion

#define INT_BASE 0x000000 // TODO
// The pending registers indicate if an interrupt of a particular type has been triggered and needs processing.
// An interrupt handler should also write to this register to clear the pending flag, once the interrupt has been processed.
#define INT_PENDING_REG0	INT_BASE		// Interrupt pending #0
#define INT_PENDING_REG1	(INT_BASE+1)	// Interrupt pending #1
#define INT_PENDING_REG2	(INT_BASE+2)	// Interrupt pending #2
#define INT_PENDING_REG3	(INT_BASE+3)	// Interrupt pending #3---FMX Model only
// This register indicates if the interrupt is triggered by a high or low signal on the input.
#define INT_POL_REG0		(INT_BASE+4)	// Interrupt polarity #0
#define INT_POL_REG1		(INT_BASE+5)	// Interrupt polarity #1
#define INT_POL_REG2 		(INT_BASE+6)	// Interrupt polarity #2
#define INT_POL_REG3 		(INT_BASE+7)	// Interrupt polarity #3---FMX Model only
// This register indicates if the interrupt is triggered by an transition (edge) or by a high or low value.
#define INT_EDGE_REG0 		(INT_BASE+8)	// Enable Edge Detection #0
#define INT_EDGE_REG1 		(INT_BASE+9)	// Enable Edge Detection #1
#define INT_EDGE_REG2 		(INT_BASE+10)	// Enable Edge Detection #2
#define INT_EDGE_REG3 		(INT_BASE+11)	// Enable Edge Detection #3---FMX Model only
// This register indicates if the associated interrupt will trigger an IRQ to the processor.
// Interrupt signals with a mask bit of 0 will be ignored, while those with a mask bit of 1 will trigger an interrupt to the CPU.
#define INT_MASK_REG0 		(INT_BASE+12)	// Enable Interrupt #0
#define INT_MASK_REG1 		(INT_BASE+13)	// Enable Interrupt #1
#define INT_MASK_REG2 		(INT_BASE+14)	// Enable Interrupt #2
#define INT_MASK_REG3		(INT_BASE+15)	// Enable Interrupt #3


#ifndef ASM_ONLY
// Only for C

#include <stdint.h>

// Send a byte to the PS/2 controller
void PS2_send_controller(const uint8_t b);

#endif // ASM_ONLY

#endif // MACHINE_C256FOENIXGENX
#endif // FOENIX
