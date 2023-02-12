#include <stdarg.h>
#include <stdio.h>
#include "foenix.h"
#include "drivers/uart16550.h"

/*
 * Output the string on the serial port.
 */
void a2560u_debugnl(const char* __restrict__ s, ...);
void a2560u_debug(const char* __restrict__ s, ...);
