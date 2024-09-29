/*
 * foenix.h - Foenix Retro System computer specific defines
 *
 * Copyright (C) 2024 The EmuTOS development team
 *
 * Authors:
 *  VB   Vincent Barrilliot
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#ifndef FOENIX_H
#define FOENIX_H

#include <stdint.h>

/* General memory map */
#define VICKY_TEXT_SIZE 0x4000 /* Size of the text memory */
#define VICKY_FONT_SIZE 0x1000 /* Size of the font memory */

#ifdef MACHINE_A2560U
  #define CPU_FREQ        20000000 /* 20Mhz : TODO get rid of this and use cpu_freq */
  #define GAVIN           0x00B00000
  #define BEATRIX         0x00B20000
  #define VICKY           0x00B40000
  #define VICKY_TEXT      0x00B60000
  #define VICKY_TEXT_MEM  0x00B60000
  #define VICKY_FONT      (VICKY+0x8000)      /* Font memory (-> 0xbff) */  

  #define VRAM_Bank0      0x00C00000
  #define VRAM0_SIZE      0x00200000 /* 2MB */
#elif defined(MACHINE_A2560X)
  #define CPU_FREQ        33333333 /* 33Mhz : TODO get rid of this and use cpu_freq */
  #define GAVIN           0xFEC00000
  #define BEATRIX         0xFEC20000
  #define VICKY_A         0xFEC40000
  #define VICKY_A_MOUSE   0xFEC40000
  #define VICKY_FONT_A    0xFEC48000
  #define VICKY_TEXT_A    0xFEC60000
  #define VICKY_B         0xFEC80000
  #define VICKY_FONT_B    0xFEC88000
  #define VICKY_TEXT_B    0xFECA0000
  // Convenience, we treat the most feature-full screen as main screen (so to share code with the U which only has 1 screen)
  #define VICKY VICKY_B
  #define VICKY_TEXT VICKY_TEXT_B
  // I have deliberately inverted bank 0 and 1 so that bank 0 is, for both the U and the X/K/GenX the "full featured" bank
  #define VRAM_Bank0      0x00800000
  #define VRAM0_SIZE      0x00400000 /* 4MB */
  #define VRAM_Bank1      0x00C00000
  #define VRAM1_SIZE      0x00400000 /* 4MB */
#endif

/* Chipset addresses */
#define BQ4802LY_BASE  (GAVIN+0x80)
#ifdef MACHINE_A2560U
#define PS2_BASE       (GAVIN+0x2800)
#elif defined(MACHINE_A2560X) || defined(MACHINE_A2560K) || defined(MACHINE_GENX)
#define PS2_BASE       0xFEC02060 // TODO: improve this with base address of SUPERIO
#endif
#define SDC_BASE       (GAVIN+0x300)
#define IDE_BASE       (GAVIN+0x400)



#define GAVIN_CTRL  (GAVIN)
  #define GAVIN_CTRL_BEEPER  0x0010
  #define GAVIN_CTRL_PWRLED  0x0001
  #define GAVIN_CTRL_DISKLED 0x0002

#ifdef MACHINE_A2560U
#define UART16550_CLOCK 20000000L /* 20Mhz, system clock */
/* Serial port speed codes for a2560u_serial_set_bps */
#define UART1       (UART16550*)(GAVIN+0x28F8)
/* For speed codes, checkout uart16550.h */
#elif defined (MACHINE_A2560X) || defined(MACHINE_A2560K)
#define UART16550_CLOCK 1843200L
#define UART1       (UART16550*)0xFEC023F8  /* Base address for UART 1 (COM1) */
#define UART2       (UART16550*)0xFEC022F8  /* Base address for UART 2 (COM2), the doc is wrong! */
#endif

/* PS2 */
#define PS2_DATA       PS2_BASE
#define PS2_CMD        (PS2_BASE+0x04)

/* Interrupts */
#define IRQ_GROUPS        3 /* Number of IRQ groups */
#define IRQ_PENDING_GRP0 (GAVIN+0x100)
    #define INT_SOF_A           0x00    /* Vicky Channel A Start of Frame */
    #define INT_SOL_A           0x01    /* Vicky Channel A Start of Line */
    #define INT_VICKY_A_1       0x02
    #define INT_VICKY_A_2       0x03
    #define INT_VICKY_A_3       0x04
    #define INT_VICKY_A_4       0x05
    #define INT_RESERVED_1      0x06
    #define INT_VICKY_A_DAC     0x07
#if defined(MACHINE_A2560X)
    #define INT_SOF_B           0x08    /* Vicky Channel B Start of Frame */
    #define INT_SOL_B           0x09    /* Vicky Channel B Start of Line */
    #define INT_VICKY_B_1       0x0A
    #define INT_VICKY_B_2       0x0B
    #define INT_VICKY_B_3       0x0C
    #define INT_VICKY_B_4       0x0D
    #define INT_RESERVED_2      0x0E
    #define INT_VICKY_B_DAC     0x0F
#endif

#define IRQ_PENDING_GRP1 (GAVIN+0x102)
    #define INT_KBD_PS2         0x10    /* PS/2 Keyboard */
    #define INT_MOUSE           0x12    /* PS/2 Mouse */
    #define INT_COM1            0x13    /* COM1 */
    #define INT_TIMER0          0x18    /* Timer 0, Clocked with the CPU Clock */
    #define INT_TIMER1          0x19    /* Timer 1, Clocked with the CPU Clock */
    #define INT_TIMER2          0x1A    /* Timer 2, Clocked with the CPU Clock */
    #define INT_TIMER3          0x1B    /* Timer 3, Clocked with the SOF Channel A */
    #define INT_TIMER4          0x1C    /* Timer 4, Clocked with the SOF Channel B */
    #define INT_RTC             0x1F    /* Real Time Clock */
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

/* 68000 Interrupt vector numbers (not address!)  */
#if defined(MACHINE_A2560X) || defined(MACHINE_A2560X)
#define INT_VICKYII_A     0x1D
#define INT_VICKYII_B     0x1E
#else
#define INT_VICKYII       0x1E
#endif
#define INT_PS2KBD_VECN   0x40
#define INT_PS2MOUSE_VECN 0x42
#define INT_COM1_VECN     0x43
#define INT_TIMERS_VECN   0x48
#define INT_TIMER0_VECN   (INT_TIMERS_VECN+0) /* Timer 0, Clocked with the CPU Clock */
#define INT_TIMER1_VECN   (INT_TIMERS_VECN+1) /* Timer 1, Clocked with the CPU Clock */
#define INT_TIMER2_VECN   (INT_TIMERS_VECN+2) /* Timer 2, Clocked with the CPU Clock */
#define INT_TIMER3_VECN   (INT_TIMERS_VECN+3) /* Timer 3, Clocked with the SOF Channel A */
#define INT_BQ4802LY_VECN 0x4F
#define INT_NIRQ          4

/* SPI/SD interface controller */
/* Transaction types for the "transfer type" register (GAVIN+0x302) */
#define SDC_TRANS_DIRECT   0
#define SDC_TRANS_SD_INIT  1
#define SDC_TRANS_SD_READ  2
#define SDC_TRANS_SD_WRITE 3
/* Set this to the transfer control (GAVIN+0x303) to start the operation set in "transfer type" */
#define SDC_TRANS_START  1
/* Bit mask to get the status of the operation (GAVIN+0x304) */
#define SDC_TRANS_BUSY   1

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
#define TIMER_CTRL_ENABLE   0x01L
#define TIMER_CTRL_CLEAR    0x02L /* Set this if counting up */
#define TIMER_CTRL_LOAD     0x04L /* Set this if counting down */
#define TIMER_CTRL_UPDOWN   0x08L /* Count up (1) or down (0) */
#define TIMER_CTRL_RECLEAR  0x10L /* Set this to auto recommence counting up */
#define TIMER_CTRL_RELOAD   0x20L /* Set this if auto recommence counting down */
#define TIMER_CTRL_IRQ      0x80L /* Set this to enable interrupts */

/* BEATRIX */
#define SN76489_CLOCK 357954500
#define SN76489_COUNT 2
#define SN76489_L     ((uint8_t*const)(BEATRIX+0x0110)) /* Left SN76489 */
#define SN76489_R     ((uint8_t*const)(BEATRIX+0x0120)) /* Right SN76489 */
#define SN76489_BOTH  ((uint8_t*const)(BEATRIX+0x0130)) /* Writes both SN76489s */

#define OPL3_PORT     ((volatile uint8_t*)(BEATRIX+0x0200)) /* Access port for the OPL3 */
#define OPM_INT_BASE  ((volatile uint8_t*)(BEATRIX+0x0C00)) /* Internal OPM base address */
#define OPN2_INT_BASE ((volatile uint8_t*)(BEATRIX+0x0A00)) /* Internal OPN2 base address */
#define WM8776_PORT   ((uint16_t*)(BEATRIX+0x0E00))         /* Mixer/codec port */

#endif /* FOENIX */
