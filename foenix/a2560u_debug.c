#include "config.h"
#include "doprintf.h" // EmuTOS

#include "a2560u_debug.h"
#include "foenix.h"
#include "uart16550.h"
#include "vicky2_txt_a_logger.h"

void outchar(int c);
void outchar(int c) {
#ifdef MACHINE_A2560U
    char ch = (char)c;
    uart16550_put(UART1,&c,1);
#elif defined(MACHINE_A2560X)
    // Serial
    char ch = (char)c;
    uart16550_put(UART2,(uint8_t*)&ch, 1);
    
 #if FOENIX_CHANNEL_A_DEBUG_PRINT
    // Channel A
    char buf[2];
    buf[0] = (char)c;
    buf[1] = '\0';
    channel_A_write(buf);
 #endif
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

void a2560u_here(void) {
#if MACHINE_A2560_DEBUG
   channel_A_write("HERE");
#endif
}

