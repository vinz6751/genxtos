/*
 * c256genx.h - C256 Foenix GenX specific functions
 *
 * Copyright (C) 2013-2021 The EmuTOS development team
 *
 * Authors:
 *  VRI   Vincent Barrilliot
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#ifndef C256GENX_H
#define C256GENX_H
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
#endif // C256GENX_H
