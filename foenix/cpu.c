/* m68k-specific functions
 * Author: Vincent Barrilliot
 */

#include <stdint.h>
#include "foenix.h"
#include "a2560_debug.h"

uint16_t cpu_has_long_frames;


uint32_t cpu_set_vector(uint16_t num, uint32_t vector)
{
    uint32_t oldvector;
    uint32_t *addr = (uint32_t *) (4L * num);
    oldvector = *addr;

    a2560_debugnl("cpu_set_vector 0x%x %d (%p) to %p", num, num, addr, vector);

    if (vector != -1) {
        *addr = vector;
    }
    return oldvector;
}


void m68k_cpu_init(void)
{
    /* Detect if the CPU has long frames. We could also detect using tests, or drive it from
     * the the system information provided by GAVIN */
#if defined(MACHINE_A2560U)
    cpu_has_long_frames = 0;
#elif defined(MACHINE_A2560K) || defined(MACHINE_A2560M) || defined(MACHINE_A2560X) || defined(MACHINE_GENX)
    cpu_has_long_frames = -1;
#endif
}
