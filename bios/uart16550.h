/*
 * uart16550.c - UART 16550 serial port driver
 *
 * Copyright (C) 2001-2021 The EmuTOS development team
 *
 * Authors:
 *  VB   Vincent Barrilliot
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#ifndef UART16550_H
#define UART16550_H

#include <stdint.h>
#include <stdbool.h>

#ifndef UART16550_CLOCK
    #define UART16550_CLOCK 20000000 /* 20Mhz */
#endif

#define UART16550_300BPS    (UART16550_CLOCK/300/16)
#define UART16550_1200BPS   (UART16550_CLOCK/1200/16)
#define UART16550_2400BPS   (UART16550_CLOCK/2400/16)
#define UART16550_4800BPS   (UART16550_CLOCK/4800/16)
#define UART16550_9600BPS   (UART16550_CLOCK/9600/16)
#define UART16550_19200BPS  (UART16550_CLOCK/19200/16)
#define UART16550_38400BPS  (UART16550_CLOCK/38400/16)
#define UART16550_57600BPS  (UART16550_CLOCK/57600/16)
#define UART16550_115200BPS (UART16550_CLOCK/115200/16)

#define UART16550_5D    0    /* Number of data bits */
#define UART16550_6D    1
#define UART16550_7D    2
#define UART16550_8D    3
#define UART16550_1S    0    /* 1 stop bit */
#define UART16550_1_5S  4    /* 1.5 stop bit (only for 5 bit data word) */
#define UART16550_2S    4    /* 2 stop bits (inly for 6,7,8 bit data word */
#define UART16550_NOPARITY 0 /* No parity */
#define UART16550_ODD   8    /* Odd parity */
#define UART16550_EVEN  0x18 /* Even parity */
#define UART16550_HIGHP 0x28 /* High parity (stick) */
#define UART16550_LOWP  0x38 /* Low parity (stick) */
#define UART16550_DBRK  0x00 /* Break signal disabled */
#define UART16550_EBRK  0x40 /* Break signal enabled */


typedef uint8_t UART16550;

void uart16550_init(UART16550 *uart);
void uart16550_set_bps(UART16550 *uart, uint16_t bps_code);
void uart16550_set_line(UART16550 *uart, uint8_t flags);
void uart16550_put(UART16550 *uart, const uint8_t *bytes, uint32_t count);
uint8_t uart16550_get(const UART16550 *uart);
bool uart16550_can_get(const UART16550 *uart);
bool uart16550_can_put(const UART16550 *uart);


#endif