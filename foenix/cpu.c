#include <stdint.h>
#include "foenix.h"
#include "a2560_debug.h"

uint32_t set_vector(uint16_t num, uint32_t vector)
{
    uint32_t oldvector;
    uint32_t *addr = (uint32_t *) (4L * num);
    oldvector = *addr;

    a2560_debugnl("set_vector 0x%x %d (%p) to %p", num, num, addr, vector);

    if (vector != -1) {
        *addr = vector;
    }
    return oldvector;
}
