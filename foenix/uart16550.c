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
#include <stdbool.h>
#include "uart16550.h"

/* DLAB=0 */
#define RBR 0 /* Receive buffer (R) */
#define THR 0 /* Transmitter holding */
#define IER 1 /* Interrupt enable (R/W) */
#define ISR 2 /* Interrupt service register (16C750) / Interrupt identification (16550) (R)*/
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

#define IER_RX      1 /* Reception interrupt */
#define IER_TX      2 /* Transmit interrupt */
#define IER_MODEM   4 /* Modem status interrupt */

#define MCR_DTR     1
#define MCR_RTS     2
#define MCR_OUT1    4
#define MCR_OUT2    8

#define R8(x) ((volatile uint8_t*)x) /* Convenience */

void a2560u_rts(uint16_t);

void (*uart16550_rx_handler)(uint8_t byte);


void uart16550_init(UART16550 *uart)
{
    uart16550_set_bps(uart, UART16550_57600BPS);
    uart16550_set_line(uart, UART16550_8D | UART16550_1S | UART16550_NOPARITY);
    uart16550_rx_handler = (void(*)(uint8_t))a2560u_rts;
    //uart[FCR] = 0b00100111; //0b00000110; // No FIFO, reset FIFOs. See 16C750B doc
    uart[FCR] = 0xC1; /* 16550: Clear FIFOs, one byte buffer */
    
    // Don't enable interrupts by default
    R8(uart)[IER] = 0;
    R8(uart)[MCR] = MCR_DTR | MCR_RTS;

    /* Flush reception */
    while (uart16550_can_get(uart))
        uart16550_get_nowait(uart);
}

/* Return the ("ideal") currently configured speed*/
uint32_t uart16550_bps_code_to_actual(uint16_t bps_code) {
    switch (bps_code) {
        case UART16550_50BPS: return 50;
        case UART16550_75BPS: return 75;
        case UART16550_110BPS: return 110;
        case UART16550_134BPS: return 134;
        case UART16550_150BPS: return 150;
        case UART16550_200BPS: return 200;
        case UART16550_300BPS: return 300;
        case UART16550_600BPS: return 600;
        case UART16550_1200BPS: return 1200;
        case UART16550_1800BPS: return 1800;
        case UART16550_2000BPS: return 2000;
        case UART16550_2400BPS: return 2400;
        case UART16550_3600BPS: return 3600;
        case UART16550_4800BPS: return 4800;
        case UART16550_9600BPS: return 9600;
        case UART16550_19200BPS: return 19200;
        case UART16550_38400BPS: return 38400;
        case UART16550_57600BPS: return 57600;
        case UART16550_115200BPS: return 115200;
        case UART16550_230400BPS: return 230400;
        default: return (UART16550_CLOCK/bps_code/16);
    }
}

/* Return the code for the speed currently set */
uint16_t uart16550_get_bps_code(const UART16550 *uart) {
    uint16_t bps_code;
 
    /* Set DLAB */
    R8(uart)[LCR] |= DLAB;

    ((uint8_t*)&bps_code)[0] = R8(uart)[DLM] = ((uint8_t*)&bps_code)[0];
    ((uint8_t*)&bps_code)[1] = R8(uart)[DLL] = ((uint8_t*)&bps_code)[1];

    /* Unset DLAB */
    R8(uart)[LCR] &= ~DLAB;
    
    return bps_code;
}

void uart16550_set_bps(UART16550 *uart, uint16_t bps_code)
{
    /* Set DLAB */
    R8(uart)[LCR] |= DLAB;

    R8(uart)[DLL] = ((uint8_t*)&bps_code)[1];
    R8(uart)[DLM] = ((uint8_t*)&bps_code)[0];

    /* Unset DLAB */
    R8(uart)[LCR] &= ~DLAB;
}


void uart16550_set_line(UART16550 *uart, uint8_t flags)
{
    R8(uart)[LCR] = flags & ~DLAB; /* Mask DLAB */
}


void uart16550_put(UART16550 *uart, const uint8_t *bytes, uint32_t count)
{
    uint8_t *c = (uint8_t*)bytes;

    while (count--)
    {        
        while (!uart16550_can_put(uart))
            ;
        R8(uart)[THR] = *c++;
    };
}


bool uart16550_can_get(const UART16550 *uart)
{    
    return R8(uart)[LSR] & 0x01;
}


bool uart16550_can_put(const UART16550 *uart)
{    
    return R8(uart)[LSR] & 0x20; /* bit 5: THR is empty */
}


uint8_t uart16550_get_nowait(const UART16550 *uart)
{
    return R8(uart)[RBR];
}


void uart16550_rx_irq_enable(UART16550 *uart, bool enable) {
    if (enable) 
    {
        uart[MCR] |= MCR_OUT2;
        uart[IER] |= IER_RX;
    }
    else
    {
        uart[IER] &= ~IER_RX;
        uart[MCR] &= ~MCR_OUT2;
    }
}