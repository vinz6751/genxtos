/*
 * foenix.h - Foenix Retro System computer specific definition.
 * 
 * This file is also used from assembly files so must not contain
 * C-related definitions.
 *
 * Copyright (C) 2025 The EmuTOS development team
 *
 * Authors:
 *  VB   Vincent Barrilliot
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#ifndef FOENIX_H
#define FOENIX_H

#if !(defined(MACHINE_A2560K) || defined(MACHINE_A2560M) || defined(MACHINE_A2560X) || defined(MACHINE_A2560U) || defined(MACHINE_GENX))
#include "../include/config.h"
#endif

/* General memory map */
#define VICKY_TEXT_SIZE 0x4000 /* Size of the text memory */
#define VICKY_FONT_SIZE 0x1000 /* Size of the font memory */
#define VICKY_MOUSE_MEM_OFFSET  0x0400
#define VICKY_MOUSE_CTRL_OFFSET 0x0C00
#define VICKY_FONT_MEM_OFFSET   0x8000
#define VICKY_TEXT_MEM_OFFSET   0x20000
#define VICKY_TEXT_COL_OFFSET   0x28000

#ifdef MACHINE_A2560U
  #define CPU_FREQ        20000000   /* 20Mhz : TODO get rid of this and use cpu_freq */
  #define SRAM_TOP        0x00400000 /* Address of first byte after SRAM */ 
  #define GAVIN           0x00B00000
  #define BEATRIX         0x00B20000
  #define VICKY           0x00B40000
  #define VICKY_TEXT      (VICKY+VICKY_TEXT_MEM_OFFSET)
  #define VICKY_TEXT_MEM  VICKY_TEXT
  #define VICKY_FONT      (VICKY+VICKY_FONT_MEM_OFFSET)      /* Font memory (-> 0xbff) */  
  #define VRAM_Bank0      0x00C00000
  #define VRAM0_SIZE      0x00200000 /* 2MB */

#elif defined(MACHINE_A2560X) || defined(MACHINE_GENX) || defined(MACHINE_A2560K) || defined(MACHINE_A2560M)
  #define CPU_FREQ        33333333   /* 33Mhz : TODO get rid of this and use cpu_freq */
  #define GAVIN           0xFEC00000
  #define BEATRIX         0xFEC20000
  #define SUPERIO_BASE    0xFEC02000

  #if defined(MACHINE_A2560X) || defined(MACHINE_GENX) || defined(MACHINE_A2560K)
  #define SRAM_TOP        0x00400000 /* Address of first byte after SRAM */
  /* I have deliberately inverted bank 0 and 1 so that bank 0 is, for both the U and the X/K/GenX the "full featured" bank */
  #define VRAM_Bank0      0x00800000
  #define VRAM0_SIZE      0x00400000 /* 4MB */
  #define VRAM_Bank1      0x00C00000
  #define VRAM1_SIZE      0x00400000 /* 4MB */

  /* Video */
  #define VICKY_A         0xFEC40000
  #define VICKY_FONT_A    (VICKY_A+VICKY_FONT_MEM_OFFSET)
  #define VICKY_TEXT_A    (VICKY_A+VICKY_TEXT_MEM_OFFSET)

  #define VICKY_B         0xFEC80000
  #define VICKY_FONT_B    (VICKY_B+VICKY_FONT_MEM_OFFSET)
  #define VICKY_TEXT_B    (VICKY_B+VICKY_TEXT_MEM_OFFSET)
  
  /* Convenience, we treat the most feature-full screen as main screen (so to share code with the U which only has 1 screen) */
  #define VICKY VICKY_B
  #define VICKY_TEXT VICKY_TEXT_B
  
  #elif defined(MACHINE_A2560M)
  #define SRAM_TOP              0x00800000 /* Address of first byte after SRAM */
  #define VICKY2                0xFEC80000 /* This is a limited "legacy" mode with no bitmap */
  #define VICKY2_FONT           (VICKY2+VICKY_FONT_MEM_OFFSET) /* 0xFEC88000 */
  #define VICKY3                0xFC000000
  #define VICKY3_CTRL           VICKY3
    #define VICKY3_CTRL_EN      0x00000001  /* Enable the bitmap background 'DDR3 Memory' */
    #define VICKY3_CTRL_MODE0   0x00000002  /* Bitmap Mode 0 */
    #define VICKY3_CTRL_MODE1   0x00000004  /* Bitmap Mode 1 */
    #define VICKY3_CTRL_MODE2   0x00000008  /* Bitmap Mode 2 */
      /* Bitmap Mode [2:0]
       * 0.0.0 :  512x768     - 1bpp
       * 0.0.1 :  512x768     - 8bpp
       * 0.1.0 :  512x768     - 16bpp
       * 0.1.1 :  512x768     - 32bpp
       * 1.0.0 :  1024x768    - 1bpp
       * 1.0.1 :  1024x768    - 8bpp
       * 1.1.0 :  1024x768    - 16bpp
       * 1.1.1 :  1024x768    - 32bpp */
  #define VICKY3_BITMAP_ADDR    (VICKY3+4) /* Address of bitmap framebuffer in the DDR3 memory */
  #define VKYIII_MONO_COLOR     (VICKY3+8) /* Color for the pixels when 1bpp mode is used */
  #define VICKY3_LUT            0xFC002000
  #define VICKY2_TEXT_MATRIX    (VICKY2+VICKY_TEXT_MEM_OFFSET)
  #define VICKY2_COLOR_MATRIX   (VICKY2+VICKY_TEXT_COL_OFFSET)

  /* Convenience for single screen */
  #define VICKY           VICKY2
  #define VICKY_FONT      VICKY2_FONT
  #define VICKY_TEXT      VICKY2_TEXT_MATRIX

  #define VRAM_Bank0      0x00800000
  #define VRAM0_SIZE      0x00400000 /* 4MB */
  #else
    #error "Define the Foenix machine"
  #endif
#endif

/* Chipset addresses */
/* Real time clock (RTC) */
#define BQ4802LY_BASE  (GAVIN+0x80)

/* PS/2 keyboard/mouse */
#ifdef MACHINE_A2560U
/* In the A2560U, there is no SuperIO so the PS2 controller is in the FPGA */
#define PS2_BASE       (GAVIN+0x2800)
#elif defined(MACHINE_A2560X) || defined(MACHINE_A2560K) || defined(MACHINE_GENX) || defined(MACHINE_A2560M)
#define PS2_BASE       (SUPERIO_BASE+0x60)
#endif

/* IDE and SDCard */
#if defined(MACHINE_A2560K) || defined(MACHINE_A2560X) || defined(MACHINE_GENX) || defined(MACHINE_A2560U)
#define IDE_BASE       (GAVIN+0x400)
#define SDC_BASE       (GAVIN+0x300)
#elif defined(MACHINE_A2560M)
#define SDC_BASE       (GAVIN+0x300) /* SD Card 0 (front panel) */
#define SDC1_BASE      (GAVIN+0x380) /* SD Card 1 (main board) */
/*#define SDC_BASE       (GAVIN+0x300)*/
/*GAVIN 518 en R32*/
#endif



#define GAVIN_CTRL              (GAVIN)
/* Bits of GAVIN's control register */
  #define GAVIN_CTRL_PWRLED  0x0001
#if defined(MACHINE_A2560M)
  #define GAVIN_CTRL_SDC1_LED   0x0002
  #define GAVIN_CTRL_SDC2_LED   0x0004
  #define GAVIN_CTRL_NET_LED    0x0008
#else
  #define GAVIN_CTRL_DISKLED    0x0002
#endif
  #define GAVIN_CTRL_BEEPER     0x0010
  #define GAVIN_CTRL_LPC_RESET  0x0100 /* Super IO Reset line */
#if defined(MACHINE_A2560K) || defined(MACHINE_A2560M) || defined(MACHINE_A2560X) || defined(MACHINE_GENX)
  #define GAVIN_CTRL_RESET      0x80000000 /* Write 0xDEAD---- to reset the machine */
#endif

#if defined(MACHINE_A2560U)
#define GAVIN_RESET         (GAVIN+0x02) /* Write 0xDEAD to reset the machine */
#endif

#if defined(MACHINE_A2560M)
/* RGB values of LEDs */
#define GAVIN_PWR_LED_COLOR  (GAVIN+0x10)
#define GAVIN_SDC1_LED_COLOR (GAVIN+0x14)
#define GAVIN_SDC2_LED_COLOR (GAVIN+0x18)
#define GAVIN_NET_LED_COLOR  (GAVIN+0x1C)
#endif

/* Serial ports */
#ifdef MACHINE_A2560U
#define UART16550_CLOCK 20000000UL /* 20Mhz, system clock */
/* Serial port speed codes for a2560_serial_set_bps */
#define UART1       (GAVIN+0x28F8)
/* For speed codes, checkout uart16550.h */
#elif defined (MACHINE_A2560X) || defined(MACHINE_A2560K) || defined(MACHINE_A2560M) || defined(MACHINE_GENX)
#define UART16550_CLOCK 1843200UL
#define UART1       (SUPERIO_BASE+0x3F8)  /* Base address for UART 1 (COM1) */
#define UART2       (SUPERIO_BASE+0x2F8)  /* Base address for UART 2 (COM2), the doc is wrong! */
#endif
#if defined(MACHINE_A2560M)
/* UART3 is in the FPGA and is emulated through the serial port,
 * it's the last of the 4 serial ports that the Foenix exposes through USB 
 * (first = JTAG, second: debug port, third: USB OTG, fourth: UART)
 * It doesn't need to be initialised, and is 8N1 115200bps by default.
 */
#define UART3_CTRL (volatile uint8_t*)0xFEC00B00 /* Control */
/* Bits:
  * 0: speed 0:115200 1: 921600 bps
  * 1: RX register empty
  * 2: TX register empty
 */
#define UART3_HIGH_SPEED 1 (0)
#define UART3_RX_EMPTY 2
#define UART3_TX_EMPTY 4
#define UART3_DATA ((volatile uint8_t*)0xFEC00B01) /* Control */
#endif

/* MIDI */
#if defined (MACHINE_A2560X) || defined(MACHINE_A2560K) || defined(MACHINE_A2560M) || defined(MACHINE_GENX)
#define MPU401_BASE       (SUPERIO_BASE+0x330)
#endif

/* PS2 */
#define PS2_DATA          PS2_BASE
#define PS2_CMD           (PS2_BASE+0x04)

/* Gavin interrupt control registers */
#define IRQ_PENDING_GRP0  (GAVIN+0x100)
#define IRQ_PENDING_GRP1  (GAVIN+0x102)
#define IRQ_PENDING_GRP2  (GAVIN+0x104)
#define IRQ_POL_GRP0      (GAVIN+0x108)
#define IRQ_POL_GRP1      (GAVIN+0x10A)
#define IRQ_POL_GRP2      (GAVIN+0x10C)
#define IRQ_EDGE_GRP0 	  (GAVIN+0x110)
#define IRQ_EDGE_GRP1 	  (GAVIN+0x112)
#define IRQ_EDGE_GRP2 	  (GAVIN+0x114)
#define IRQ_MASK_GRP0 	  (GAVIN+0x118)
#define IRQ_MASK_GRP1 	  (GAVIN+0x11A)
#define IRQ_MASK_GRP2 	  (GAVIN+0x11C)

#define IRQ_GROUPS        3 /* Number of IRQ groups */

/* Gives the bit number corresponding to the given INT_xxx interrupt */
#define INT_BIT(irqn)       (irqn&0x0f)
/* Gives the IRQ_PENDING_register corresponding to the INT_xxx interrupt */
#define INT_GRP(irqn)       (IRQ_PENDING_GRP0 + 2*((irqn&0xf0)>>4))

#if defined(MACHINE_A2560M)
  /* Interrupt group 0 */
  #define INT_SOF_A           0x00    /* Vicky Channel A Start of Frame */
  #define INT_SOL_A           0x01    /* Vicky Channel A Start of Line */
  #define INT_KBD_PS2         0x08    /* PS/2 Keyboard */
  #define INT_MOUSE           0x09    /* PS/2 Mouse */
  #define INT_COM1            0x0A    /* COM1 */
  #define INT_COM2            0x0B    /* COM2 */
  #define INT_FDC             0x0C    /* Floppy controller */
  #define INT_LPT             0x0D    /* Parallel port */
  #define INT_MIDI            0x0E    /* MPU-401 / MIDI */
  #define INT_RTC             0x0F    /* Real Time Clock */
  /* Interrupt group 1 */
  #define INT_SDCARD_INSERT   0x18
  #define INT_TIMER0          0x19    /* Timer 0, Clocked with the CPU Clock */
  #define INT_TIMER1          0x1A    /* Timer 1, Clocked with the CPU Clock */
  #define INT_TIMER2          0x1B    /* Timer 2, Clocked with the CPU Clock */
  #define INT_TIMER3          0x1C    /* Timer 3, Clocked with the SOF */

#else /* !defined(MACHINE_A2560M) */
  /* Interrupt group 0 */
  #define INT_SOF_A           0x00    /* Vicky Channel A Start of Frame */
  #define INT_SOL_A           0x01    /* Vicky Channel A Start of Line */
  #define INT_VICKY_A_1       0x02
  #define INT_VICKY_A_2       0x03
  #define INT_VICKY_A_3       0x04
  #define INT_VICKY_A_4       0x05
  #define INT_RESERVED_1      0x06
  #define INT_VICKY_A_DAC     0x07
  #if defined(MACHINE_A2560X) || defined(MACHINE_A2560K) || defined(MACHINE_GENX)
    #define INT_SOF_B         0x08    /* Vicky Channel B Start of Frame */
    #define INT_SOL_B         0x09    /* Vicky Channel B Start of Line */
    #define INT_VICKY_B_1     0x0A
    #define INT_VICKY_B_2     0x0B
    #define INT_VICKY_B_3     0x0C
    #define INT_VICKY_B_4     0x0D
    #define INT_RESERVED_2    0x0E
    #define INT_VICKY_B_DAC   0x0F
  #endif
  /* Interrupt group 1 */
  #define INT_KBD_PS2         0x10    /* PS/2 Keyboard */
  #if defined(MACHINE_A2560K)
    #define INT_MAURICE_PS2   0x11    /* Maurice (A2560K keyboard controller) */
  #endif
  #define INT_MOUSE           0x12    /* PS/2 Mouse */
  #define INT_COM1            0x13    /* COM1 */
  #if defined (MACHINE_A2560X) || defined(MACHINE_A2560K) || defined(MACHINE_GENX)
    #define INT_COM2          0x14    /* COM2 */
    #define INT_LPT           0x15    /* Parallel port */
    #define INT_FDC           0x16    /* Floppy controller */
    #define INT_MIDI          0x17    /* MPU-401 / MIDI */
  #endif
  #define INT_TIMER0          0x18    /* Timer 0, Clocked with the CPU Clock */
  #define INT_TIMER1          0x19    /* Timer 1, Clocked with the CPU Clock */
  #define INT_TIMER2          0x1A    /* Timer 2, Clocked with the CPU Clock */
  #define INT_TIMER3          0x1B    /* Timer 3, Clocked with the SOF Channel A */
  #define INT_TIMER4          0x1C    /* Timer 4, Clocked with the SOF Channel B */
  #define INT_RTC             0x1F    /* Real Time Clock */
#endif /* defined(MACHINE_A2560M) */


/* 68000 Interrupt vector numbers (not addresses ! The address is vector number * 4) */
#if defined(MACHINE_A2560K) || defined(MACHINE_A2560X) || defined(MACHINE_GENX)
  #define INT_VICKYII_A     0x1D
  #define INT_VICKYII_B     0x1E
#elif defined(MACHINE_A2560U) || defined(MACHINE_A2560M)
  #define INT_VICKYII       0x1E
#endif
#if defined(MACHINE_A2560M)
  /* See FoenixMCP/src/m68k/startup_m68k.s */
  #define INT_PS2KBD_VECN   0x40
  #define INT_PS2MOUSE_VECN 0x41
  #define INT_COM1_VECN     0x42
  #define INT_COM2_VECN     0x43
  #define INT_LPT_VECN      0x44
  #define INT_FDC_VECN      0x45
  #define INT_MIDI_VECN     0x46
  #define INT_BQ4802LY_VECN 0x47

  #define INT_SDCARD_INSERT_VECN 0x50
  #define INT_TIMER0_VECN   0x51 /* Timer 0, Clocked with the CPU Clock */
  #define INT_TIMER1_VECN   0x52 /* Timer 1, Clocked with the CPU Clock */
  #define INT_TIMER2_VECN   0x53 /* Timer 2, Clocked with the CPU Clock */
  #define INT_TIMER3_VECN   0x54 /* Timer 3, Clocked with the SOF */

  #define INT_VIA0_VECN             0x55
  #define INT_VIA1_VECN             0x56
  #define INT_WIZFI360_FIFO_RX_VECN 0x57
  #define INT_WS6100_VECN           0x58
  #define INT_WIZFI360_VECN         0x59
  #define INT_CARTRIDGE_VECN        0x60

  #else /* !defined(MACHINE_A2560M) */
  #define INT_PS2KBD_VECN   0x40
  #define INT_MAURICE_VECN  0x41
  #define INT_PS2MOUSE_VECN 0x42
  #define INT_COM1_VECN     0x43
  #define INT_COM2_VECN     0x44
  #define INT_LPT_VECN      0x45
  #define INT_FDC_VECN      0x46
  #define INT_MIDI_VECN     0x47
  #define INT_TIMER0_VECN   0x48 /* Timer 0, Clocked with the CPU Clock */
  #define INT_TIMER1_VECN   0x49 /* Timer 1, Clocked with the CPU Clock */
  #define INT_TIMER2_VECN   0x4A /* Timer 2, Clocked with the CPU Clock */
  #define INT_TIMER3_VECN   0x4B /* Timer 3, Clocked with the SOF Channel A */
  #define INT_BQ4802LY_VECN 0x4F
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
#define TIMER_CTRL_ENABLE   0x01L /* Set this to 1 to star the timer */
#define TIMER_CTRL_CLEAR    0x02L /* Set this to 1 temporarily to force the timer's counter to 0 (must then be 0 for normal operation)*/
#define TIMER_CTRL_LOAD     0x04L /* Set this to 1 to load the compare if counting down (then must be 0 for normal operation) */
#define TIMER_CTRL_UPDOWN   0x08L /* Count up (1) or down (0) */
#define TIMER_CTRL_RECLEAR  0x10L /* Set this to auto recommence counting up */
#define TIMER_CTRL_RELOAD   0x20L /* Set this if auto recommence counting down */
#define TIMER_CTRL_IRQ      0x80L /* Set this to enable interrupts (if 0, interrupt will never be pending) */


/* BEATRIX */
#if defined(MACHINE_A2560M)
#define SN76489_CLOCK 5632000UL /* Found by trial/error, it seems like it's an issue that will be corrected */
#else
#define SN76489_CLOCK 3579545UL
#endif
#define SN76489_COUNT 2
#define SN76489_L     (BEATRIX+0x0110) /* Left SN76489 */
#define SN76489_R     (BEATRIX+0x0120) /* Right SN76489 */
#define SN76489_BOTH  (BEATRIX+0x0130) /* Writes both SN76489s */

#define OPL3_PORT     (BEATRIX+0x0200) /* Access port for the OPL3 */
#define OPM_INT_BASE  (BEATRIX+0x0C00) /* Internal OPM base address */
#define OPN2_INT_BASE (BEATRIX+0x0A00) /* Internal OPN2 base address */
#define WM8776_PORT   (BEATRIX+0x0E00) /* Mixer/codec port */


/* SD Card controller */
#if defined(MACHINE_A2560M)
  #define GAVIN_SDC_PRESENT   0x01000000  /* Is an SD card present? --- 0:Yes, 1:No */
  #define GAVIN_SDC_WPROT     0x02000000  /* Is the SD card write protected? --- 0:Yes, 1:No */
  #define GAVIN_BOOTMODE_0    0x00000001  /* DIP switch: boot mode 0 */
  #define GAVIN_BOOTMODE_1    0x00000002  /* DIP switch: boot mode 1 */
  #define GAVIN_DIP_USER_0    0x00000100  /* DIP switch: User 0 */
  #define GAVIN_DIP_USER_1    0x00000200  /* DIP switch: User 1 */
#endif

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


#endif /* FOENIX */
