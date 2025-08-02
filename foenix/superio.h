/*
 * Define the SuperIO control registers
 *
 * See official documentation on the LPC47M107S for details on what
 * these registers all do:
 * https://ww1.microchip.com/downloads/en/DeviceDoc/47m10x.pdf
 * Credits to FoeniMCP source code
 */

#ifndef __SUPERIO_H
#define __SUPERIO_H

#include <stdint.h>



void superio_init(void);


#define CONFIG_0x2E_REG ((volatile uint8_t *)SUPERIO_BASE+0x02E)
#define CONFIG_0x2F_REG ((volatile uint8_t *)SUPERIO_BASE+0x02F)

#define PME_STS_REG 	((volatile uint8_t *)SUPERIO_BASE+0x100)
#define PME_EN_REG 		((volatile uint8_t *)SUPERIO_BASE+0x102)

#define PME_STS1_REG	((volatile uint8_t *)SUPERIO_BASE+0x104)
#define PME_STS2_REG	((volatile uint8_t *)SUPERIO_BASE+0x105)
#define PME_STS3_REG	((volatile uint8_t *)SUPERIO_BASE+0x106)
#define PME_STS4_REG	((volatile uint8_t *)SUPERIO_BASE+0x107)
#define PME_STS5_REG	((volatile uint8_t *)SUPERIO_BASE+0x108)

#define PME_EN1_REG		((volatile uint8_t *)SUPERIO_BASE+0x10A)
#define PME_EN2_REG		((volatile uint8_t *)SUPERIO_BASE+0x10B)
#define PME_EN3_REG		((volatile uint8_t *)SUPERIO_BASE+0x10C)
#define PME_EN4_REG		((volatile uint8_t *)SUPERIO_BASE+0x10D)
#define PME_EN5_REG		((volatile uint8_t *)SUPERIO_BASE+0x10E)

#define SMI_STS1_REG	((volatile uint8_t *)SUPERIO_BASE+0x110)
#define SMI_STS2_REG	((volatile uint8_t *)SUPERIO_BASE+0x111)
#define SMI_STS3_REG	((volatile uint8_t *)SUPERIO_BASE+0x112)
#define SMI_STS4_REG	((volatile uint8_t *)SUPERIO_BASE+0x113)
#define SMI_STS5_REG	((volatile uint8_t *)SUPERIO_BASE+0x114)

#define SMI_EN1_REG		((volatile uint8_t *)SUPERIO_BASE+0x116)
#define SMI_EN2_REG		((volatile uint8_t *)SUPERIO_BASE+0x117)
#define SMI_EN3_REG		((volatile uint8_t *)SUPERIO_BASE+0x118)
#define SMI_EN4_REG		((volatile uint8_t *)SUPERIO_BASE+0x119)
#define SMI_EN5_REG		((volatile uint8_t *)SUPERIO_BASE+0x11A)

#define MSC_ST_REG			((volatile uint8_t *)SUPERIO_BASE+0x11C)
#define FORCE_DISK_CHANGE   ((volatile uint8_t *)SUPERIO_BASE+0x11E)
#define FLOPPY_DATA_RATE	((volatile uint8_t *)SUPERIO_BASE+0x11F)

#define UART1_FIFO_CTRL_SHDW ((volatile uint8_t *)SUPERIO_BASE+0x120)
#define UART2_FIFO_CTRL_SHDW	((volatile uint8_t *)SUPERIO_BASE+0x121)
#define DEV_DISABLE_REG		((volatile uint8_t *)SUPERIO_BASE+0x122)

#define GP10_REG			((volatile uint8_t *)SUPERIO_BASE+0x123)
#define GP11_REG			((volatile uint8_t *)SUPERIO_BASE+0x124)
#define GP12_REG			((volatile uint8_t *)SUPERIO_BASE+0x125)
#define GP13_REG			((volatile uint8_t *)SUPERIO_BASE+0x126)
#define GP14_REG			((volatile uint8_t *)SUPERIO_BASE+0x127)
#define GP15_REG			((volatile uint8_t *)SUPERIO_BASE+0x128)
#define GP16_REG			((volatile uint8_t *)SUPERIO_BASE+0x129)
#define GP17_REG			((volatile uint8_t *)SUPERIO_BASE+0x12A)

#define GP20_REG			((volatile uint8_t *)SUPERIO_BASE+0x12B)
#define GP21_REG			((volatile uint8_t *)SUPERIO_BASE+0x12C)
#define GP22_REG			((volatile uint8_t *)SUPERIO_BASE+0x12D)
#define GP23_REG			((volatile uint8_t *)SUPERIO_BASE+0x12E)
#define GP24_REG			((volatile uint8_t *)SUPERIO_BASE+0x12F)
#define GP25_REG			((volatile uint8_t *)SUPERIO_BASE+0x130)
#define GP26_REG			((volatile uint8_t *)SUPERIO_BASE+0x131)
#define GP27_REG			((volatile uint8_t *)SUPERIO_BASE+0x132)

#define GP30_REG			((volatile uint8_t *)SUPERIO_BASE+0x133)
#define GP31_REG			((volatile uint8_t *)SUPERIO_BASE+0x134)
#define GP32_REG			((volatile uint8_t *)SUPERIO_BASE+0x135)
#define GP33_REG			((volatile uint8_t *)SUPERIO_BASE+0x136)
#define GP34_REG			((volatile uint8_t *)SUPERIO_BASE+0x137)
#define GP35_REG			((volatile uint8_t *)SUPERIO_BASE+0x138)
#define GP36_REG			((volatile uint8_t *)SUPERIO_BASE+0x139)
#define GP37_REG			((volatile uint8_t *)SUPERIO_BASE+0x13A)

#define GP40_REG			((volatile uint8_t *)SUPERIO_BASE+0x13B)
#define GP41_REG			((volatile uint8_t *)SUPERIO_BASE+0x13C)
#define GP42_REG			((volatile uint8_t *)SUPERIO_BASE+0x13D)
#define GP43_REG			((volatile uint8_t *)SUPERIO_BASE+0x13E)

#define GP50_REG			((volatile uint8_t *)SUPERIO_BASE+0x13F)
#define GP51_REG			((volatile uint8_t *)SUPERIO_BASE+0x140)
#define GP52_REG			((volatile uint8_t *)SUPERIO_BASE+0x141)
#define GP53_REG			((volatile uint8_t *)SUPERIO_BASE+0x142)
#define GP54_REG			((volatile uint8_t *)SUPERIO_BASE+0x143)
#define GP55_REG			((volatile uint8_t *)SUPERIO_BASE+0x144)
#define GP56_REG			((volatile uint8_t *)SUPERIO_BASE+0x145)
#define GP57_REG			((volatile uint8_t *)SUPERIO_BASE+0x146)

#define GP60_REG			((volatile uint8_t *)SUPERIO_BASE+0x147)
#define GP61_REG			((volatile uint8_t *)SUPERIO_BASE+0x148)

#define GP1_REG				((volatile uint8_t *)SUPERIO_BASE+0x14B)
#define GP2_REG				((volatile uint8_t *)SUPERIO_BASE+0x14C)
#define GP3_REG				((volatile uint8_t *)SUPERIO_BASE+0x14D)
#define GP4_REG				((volatile uint8_t *)SUPERIO_BASE+0x14E)
#define GP5_REG				((volatile uint8_t *)SUPERIO_BASE+0x14F)
#define GP6_REG				((volatile uint8_t *)SUPERIO_BASE+0x150)

#define FAN1_REG			((volatile uint8_t *)SUPERIO_BASE+0x156)
#define FAN2_REG			((volatile uint8_t *)SUPERIO_BASE+0x157)
#define FAN_CTRL_REG		((volatile uint8_t *)SUPERIO_BASE+0x158)
#define FAN1_TACH_REG		((volatile uint8_t *)SUPERIO_BASE+0x159)
#define FAN2_TACH_REG		((volatile uint8_t *)SUPERIO_BASE+0x15A)
#define FAN1_PRELOAD_REG	((volatile uint8_t *)SUPERIO_BASE+0x15B)
#define FAN2_PRELOAD_REG	((volatile uint8_t *)SUPERIO_BASE+0x15C)

#define LED1_REG			((volatile uint8_t *)SUPERIO_BASE+0x15D)
#define LED2_REG			((volatile uint8_t *)SUPERIO_BASE+0x15E)
#define KEYBOARD_SCAN_CODE	((volatile uint8_t *)SUPERIO_BASE+0x15F)

#endif
