/* Trap interface to this library's functions
 * Author: Vincent Barrilliot
 */

#include <stdint.h>
#include "a2560_debug.h"
#include "cpu.h"
#include "trap_bindings.h" /* TRAP_NUMBER */


#define VECTOR_NUMBER (32+TRAP_NUMBER)

/* Some useful definitions */
#ifndef PACKED
# if defined(__GNUC__)
#  define PACKED __attribute__((packed))
# else
#  define PACKED
# endif
#endif
#ifndef IRQ_HANDLER
# if defined(__GNUC__)
#  define IRQ_HANDLER __attribute__((interrupt_handler))
# else
#  define IRQ_HANDLER
# endif
#endif


/* Previous trap handler */
uint32_t old_handler;

/* The IRQ handler, it deals with the CPU-specific stuff and hands over to the C dispatcher */
int32_t trap_irq_handler(uint16_t stack_frame_start);

/* Root dispatcher */
int32_t trap_dispatch(void *arguments);


void trap_init(void)
{
    old_handler = (uint32_t)cpu_set_vector(VECTOR_NUMBER, (uint32_t)trap_irq_handler);
}


void trap_exit(void)
{
    uint32_t current_handler = (uint32_t)cpu_set_vector(VECTOR_NUMBER, -1L);
    if (current_handler == (uint32_t)trap_irq_handler) {
        cpu_set_vector(VECTOR_NUMBER, old_handler);
    }
}
