#include "../include/doprintf.h" /* EmuTOS */

#include "a2560_debug.h"
#include "foenix.h"
#include "regutils.h"
#include "uart16550.h"
#include "vicky2_txt_a_logger.h"

void outchar(int c);
void outchar(int c) {
    char ch = (char)c;
#ifdef MACHINE_A2560U
    uart16550_put((UART16550*)UART1,(uint8_t*)&ch,1);
#elif defined(MACHINE_A2560K) || defined(MACHINE_A2560X) || defined(MACHINE_GENX)
 #if FOENIX_CHANNEL_A_DEBUG_PRINT
    /* Channel A */
    char buf[2];
    buf[0] = (char)c;
    buf[1] = '\0';
    channel_A_write(buf);
 #else
    uart16550_put((UART16550*)UART2,(uint8_t*)&ch, 1);
 #endif
#elif defined(MACHINE_A2560M)
    while ((R8(UART3_CTRL) & UART3_TX_EMPTY) == 0);
   /*     __asm(""); */ /* Don't optimize out */
    R8(UART3_DATA) = c;
#else
 #error "Foenix machine not specified"
#endif
}

/*
 * Output the string on the serial port.
 */
void a2560_debugnl(const char* __restrict__ fmt, ...)
{
#if MACHINE_A2560_DEBUG
{
    va_list ap;
    va_start(ap, fmt);
    doprintf(outchar, fmt, ap);
    va_end(ap);
    doprintf(outchar, "\r\n", 0L);
}
#endif
}

void a2560_debug(const char* __restrict__ fmt, ...)
{
#if MACHINE_A2560_DEBUG
    va_list ap;
    va_start(ap, fmt);
    doprintf(outchar, fmt, ap);
    va_end(ap);
#endif
}

void a2560_here(void) {
#if MACHINE_A2560_DEBUG
   a2560_debugnl("HERE");
#endif
}

