#include <stdarg.h>
#include <stdio.h>
#include "foenix.h"
#include "fnxlog.h"
#include "drivers/uart16550.h"

/*
 * Output the string on the serial port.
 */
void a2560u_debugnl(const char* __restrict__ s, ...)
{
    char msg[80];
    int length;
    char *c;
    va_list ap;
    va_start(ap, s);
    sprintf(msg,s,ap);
    va_end(ap);

    c = (char*)msg;
    while (*c++)
        ;
    length = c - msg -1;

    uart16550_put(UART0, (uint8_t*const)msg, length);
    uart16550_put(UART0, (uint8_t*)"\r\n", 2);
}

void a2560u_debug(const char* __restrict__ s, ...)
{
    char msg[80];
    int length;
    char *c;
    va_list ap;
    va_start(ap, s);
    sprintf(msg,s,ap);
    va_end(ap);

    c = (char*)msg;
    while (*c++)
        ;
    length = c - msg -1;

    uart16550_put(UART0, (uint8_t*const)msg, length);
    uart16550_put(UART0, (uint8_t*)"\r\n", 2);
}
