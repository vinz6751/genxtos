// TOS Assembler routines for the Foenix Retro Systems A2560U

#include "config.h"
#if defined(MACHINE_A2560U) || defined(MACHINE_A2560K) || defined(MACHINE_A2560M) || defined(MACHINE_A2560X) || defined(MACHINE_GENX)

#include "../foenix/foenix.h"

| Exports ---------------------------------------------------------------------
    .GLOBAL a2560_memconf


a2560_memconf: // Called from memory.S to detect memory setup
    move.l  #SRAM_TOP,d5   // this is phystop - the end of RAM on A2560U. TODO: detect U+ (4MB)
    moveq   #0,d6          // fake memctrl
    jmp     (a6)           // Return


#endif
