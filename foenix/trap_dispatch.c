/* Dispatcher for the trap interface, leverages the "dispatch.h" definitions and is called from the trap.c file
 * It's called "root" because I think it could be extended to support multiple dispatch tables for different libraries.
 * Author: Vincent Barrilliot
 */

#include "dispatch.h"

/* Some useful definitions */
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]) + __must_be_array(arr))
#endif

const struct dispatch_entry_t root_dispatch_table[] = {
    { { 0, 1 }, 0L, "dispatch_table", "Returns the dispatcher address. " },
    { { 1, 1 }, 0L, "functions_count", "Returns the number of functions." },
    { { 2, 1 }, 0L, "function_lookup", "Returns the name of the function." },
};


int32_t trap_dispatch(void *arguments)
{
    uint16_t *args = (uint16_t *)arguments;
    uint16_t function_number = *args++;
    uint16_t version = *args++;

    // Check that the function code is valid
    if (function_number >= ARRAY_SIZE(root_dispatch_table)) {
        return -1;
    }

    switch (function_number) {
        case 0:
            return (int32_t)trap_dispatch;
        case 1:
            return (int32_t)ARRAY_SIZE(root_dispatch_table);
        default:
            return -1;
    }
}

