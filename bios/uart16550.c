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

#include <stdint.h>
#include "uart16550.h"

/* DLAB=0 */
#define RBR 0 /* Receive buffer (R) */
#define THR 0 /* Transmitter holding */
#define IER 1 /* Interrupt enable (R/W) */
#define IIR 2 /* Interrupt identification (R)*/
#define FCR 2 /* FIFO control (W)*/
#define LCR 3 /* Line control (R/W) */
#define MCR 4 /* Modem control (R/W) */
#define LSR 5 /* Line status (R) */
#define MSR 6 /* Modem status (R) */
#define SCR 7 /* Scratch */
/* DLAB=1 */
#define DLL 0 /* Divisor latch LSB */
#define DLM 1 /* Divisor latch MSB */

#define DLAB  0x80           /* Enable DLL DLM access in FCR */


#define R8(x) ((volatile uint8_t*)x) /* Convenience */


void uart16550_init(UART16550 *uart)
{
    uart[MCR] = 8; /* For legacy reasons */
    uart[FCR] = 1;
    uart16550_set_bps(uart, UART16550_9600BPS);
    uart16550_set_line(uart, UART16550_8D | UART16550_1S | UART16550_NOPARITY);
    
    /* Flush reception */
    while (uart16550_can_get(uart))
        uart16550_get(uart);
}


void uart16550_set_bps(UART16550 *uart, uint16_t bps_code)
{
    /* Set DLAB */
    uart[LCR] |= DLAB;   

    uart[DLL] = ((uint8_t*)&bps_code)[1];
    uart[DLM] = ((uint8_t*)&bps_code)[0];

    /* Unset DLAB */
    uart[LCR] &= ~DLAB;
}


void uart16550_set_line(UART16550 *uart, uint8_t flags)
{
    R8(uart)[LCR] = flags & ~DLAB; /* Mask DLAB */
}


void uart16550_put(UART16550 *uart, const uint8_t *bytes, uint32_t count)
{
    uint8_t *c = (uint8_t*)bytes;
    *((unsigned long * volatile)0xB40008) = 0x0000000;
    while (count--)
    {        
        while (!uart16550_can_put(uart)) /* 5: THR is empty */
            ; 
        R8(uart)[THR] = *c++;
    };    
}


int uart16550_can_get(const UART16550 *uart)
{    
    return R8(uart)[LSR] & 1;
}

int uart16550_can_put(const UART16550 *uart)
{    
    return R8(uart)[LSR] & 0x20;
}


uint8_t uart16550_get(const UART16550 *uart)
{
    while (!uart16550_can_get(uart))
        ;
    return R8(uart)[RBR];
}
