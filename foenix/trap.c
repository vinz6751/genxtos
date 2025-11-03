/* Trap interface to this library's functions
 * Author: Vincent Barrilliot
 */

#include "cpu.h"


#define TRAP_NUMBER 15
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


/* Describes the stack frame for the trap handler. 608010+ have an extra word bug the trap handler discard it */
struct PACKED trap_stack_frame_t  {
    uint16_t status_register;
    uint32_t return_address;
    void     *arguments[];
};

/* Previous trap handler */
uint32_t old_handler;

/* The IRQ handler, it deals with the CPU-specific stuff and hands over to the dispatcher */
static int32_t IRQ_HANDLER trap_irq_handler(uint16_t stack_frame_start);

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


static int32_t IRQ_HANDLER trap_irq_handler(uint16_t stack_frame_data)
{
    uint16_t *stack_frame_start = &stack_frame_data;
 
    if (cpu_has_long_frames)
        stack_frame_start++; /* Skip the format word */

    struct trap_stack_frame_t *stack_frame = (struct trap_stack_frame_t *)stack_frame_start;

    // If we were in user mode, the arguments are from the user stack
    uint16_t *arguments = (uint16_t*)((stack_frame->status_register & 0x2000)
        ? stack_frame->arguments
        : (void*)get_usp());

    return trap_dispatch(arguments);
}
