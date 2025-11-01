/* Definitions for managing dispatch tables of functions of this library
 * Author: Vincent Barrilliot
 */

#ifndef _FOENIX_DISPATCH_H
#define _FOENIX_DISPATCH_H

#include <stdint.h>

#ifndef PACKED
# if defined(__GNUC__)
#  define PACKED __attribute__((packed))
# else
#  define PACKED
# endif
#endif

/* Dispatch table entry, each entry be 16 bytes long */
struct dispatch_entry_t {
    struct {
        uint16_t function_code;
        uint16_t version;
    } PACKED id;
    void    (*handler)(void);
    char    *name;
    char    *description;
};

/* Compile-time check that DISPATCH_ENTRY_SIZE is exactly 16 */
typedef char __dispatch_entry_size_check[((sizeof(struct dispatch_entry_t)) == 16) ? 1 : -1];

#endif
