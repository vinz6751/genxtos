/*
 * kbdvec_stubs.c - cdecl no-op handlers for default KBDVECS slots
 */

#include "emutos.h"
#include "asm.h"

void ARGS_ON_STACK kbdvec_nop_b(UBYTE data)
{
    (void)data;
}

void ARGS_ON_STACK kbdvec_nop_p(UBYTE *buf)
{
    (void)buf;
}
