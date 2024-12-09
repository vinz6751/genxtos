/* startup_c.c - C portion of the EmuTOS startup code
 *
 * Copyright (C) 2024 by Authors:
 *
 * Authors:
 *  VB   Vincent Barrilliot
 *  With credits to EmuTOS authors
 *
 * This file is not part of the original EmuTOS distribution.
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#include "emutos.h"
#include "amiga.h"
#include "bios.h"
#include "cartridge_atari.h"
#include "coldfire.h"
#include "m68k.h"
#include "memory.h"
#include "videl.h"


void startup(void);        // The purpose of this file
void startup_stage2(void); // After memory has been configured

// Some methods here we want to always inline to prevent the use of the stack for as long as it's not guaranteed to be functional
static ALWAYS_INLINE void asap(void);
static ALWAYS_INLINE void ensure_minimum_ram_setup(void);

static void reset_cpu_peripherals(void);
static void initialize_cpu(void);
static void run_user_reset_code(void);
static void run_diagnostics_cartridge(void);
static void jmp_to_with_return_address_in_a6(PFVOID function);
static void return_to_a6(void);


// called from startup.S
// This a macro function for clarity, individual functions called here only take the actions if applicable.
void startup(void) {

    asap();

    ensure_minimum_ram_setup();
    // From here on, the stack is functional and we can use normal calling conventions or exception vectors

#ifdef __mcoldfire__
    coldfire_early_init(); // TODO: what is this, and where does it really belong ?
#endif

    // Reset the VBR, it may have been modified before emutos-ram or soft reset
    m68k_set_vbr((void*)0L);

    // Reset the external hardware
    reset_cpu_peripherals();

    run_diagnostics_cartridge();

    initialize_cpu();

    run_user_reset_code();

    meminit(startup_stage2);
    // meminit returns into startup_stage2 in a hardcoded way, because we can't rely on the stack or 
    // registes to remember a return address: all the memory is clobbererd and registers as well.
    // Note: I tried using the blitter's halftone memory as temporary memory but that didn't work.

    // IE DONT PUT ANYTHING HERE BECAUSE IT WILL NEVER BE CALLED !
}



// This is run first thing, and can be used to initialize stuff that must be as very top priority.
// If these can be done later, they doesn't belong here.
// At this point the stack / memory may not be functiononal yet !
// So functions must jump back to the address provided in a6
static ALWAYS_INLINE void asap(void) {
#if defined(MACHINE_A2560X) && 0 // For debugging
    *((LONG*)0xfec00000) = 19; // Buzzer + all leds
    *((LONG*)0xfec80008) = 0;  // Black border
#endif

#ifdef MACHINE_AMIGA
    // It is *mandatory* to call this as soon as possible, to do early initialization of the Amiga hardware.
    // On cold boot, address 0 points to the ROM. After that, it will point to the RAM, as expected.
    jmp_to_with_return_address_in_a6(amiga_startup);
#endif
}


/* Ensure we have a minimal RAM setup so the RAM can be used for exception vectors and stack */
void ALWAYS_INLINE ensure_minimum_ram_setup(void) {
#if (!EMUTOS_LIVES_IN_RAM) && (CONF_WITH_ST_MMU)
    // Some ST MMUs power up with an invalid memory bank configuration,
    // inhibiting any RAM access. However, EmuTOS needs at least a
    // little bit of RAM for the CPU to read the vectors of the
    // exceptions that are raised below during hardware detection.
    // At such an early stage we don't know whether we are even running
    // on an ST. Also we can't use bus errors to detect the machine type.
    // Thus, check the MMU configuration register for values that
    // are invalid on an ST and only then write a new (valid) value
    // into it. This should not interfere with non-ST machines.

    // 0xc and above are an invalid memory bank 0 configuration on the ST.
    // 0x5 is a valid value for the ST and the TT, the correct configuration
    // will be found and applied in memory.S. On the Falcon this is a dummy
    // register, so we can write to it as well without consequences.

    if (*((UBYTE*)ST_MMU_CONFIGURATION) >= 0xc)
        *((UBYTE*)ST_MMU_CONFIGURATION) = 5;
#endif  

#ifdef __mcoldfire__
    // This has two parts: one to setup a stack (so C code can be called) and the other as C code
    jmp_to_with_return_address_in_a6(coldfire_startup);
#endif
}


static void run_diagnostics_cartridge(void) {
#if CONF_WITH_CARTRIDGE
    if (*((ULONG*)ATARI_CARTRIDGE_BASE) == 0xfa52235fL) {
#if EMUTOS_LIVES_IN_RAM
        if (*((ULONG*)(&ATARI_CARTRIDGE_BASE[0x30/sizeof(ULONG)])) != 0x45544f53/*ETOS*/)
#endif
        {
            // Don't boot the EmuTOS cartridge if EmuTOS is running in RAM
            cartridge_atari_boot();
        }
    }
#endif
}


#if CONF_WITH_VIDEL
static void falcon_reset(void) {
    (void)*((volatile WORD *)VIDEL_MONITOR_TYPE);
    m68k_reset();
    (void)*((volatile WORD *)VIDEL_MONITOR_TYPE);
}
#endif


/* Tell the CPU to send a RESET signal to the peripherals */
static void reset_cpu_peripherals(void) {
#if CONF_WITH_TT_MMU
    // TT TOS does the following even before reset; it is apparently required for correct floppy functioning
    // when EmuTOS is ROM-resident (report by Ingo Uhlemann).
    // It _should_ be harmless on all Atari systems; it has been tested (via emutos.prg) on ST/TT/Falcon.
    *((volatile WORD*)0xffff8606) = 0x0100;  // Toggle the ACSI DMA write bit
    *((volatile WORD*)0xffff8606) = 0x0000;
#endif

    // We don't need to call reset on Amiga because either this is a cold boot
    // or reset has been called just before amiga_main
#if CONF_WITH_VIDEL
    if (m68k_do_ignoring_exceptions(falcon_reset) == FALSE) {
        // If got a bus error, it's not a Falcon
        m68k_reset();
    }
#elif !defined(MACHINE_AMIGA)
    m68k_reset();
#endif
}


static void initialize_cpu(void) {
#if !defined(__mcoldfire__) && !defined(MACHINE_LISA)
    m68k_do_ignoring_exceptions(m68k_disable_caches);
    m68k_disable_mmu();
#endif
}


// Users can set a vector and a magic number to indicate that the vector is valid and should be run after a reset
// These 2 are TOS variables
extern ULONG resvalid;
extern PFVOID resvector;
static void run_user_reset_code(void) {
    for (;;) {
        if (resvalid != 0x31415926)
            return;

        if (!resvector || ((ULONG)resvector & 1)) {
            // Null vector or odd address -> ignore
            return;
        }

        jmp_to_with_return_address_in_a6(resvector);
        // So the reset routine must explicitly clear resvalid before returning
        // to a6, otherwise there will be an infinite loop.
    }
}

// Before the stack is functional we cannot call functions the normal way because the return
// address is stored on the stack. This provides an alternative method: it passes the return
// address in a6 and calls the function. The function must return by jumping back to a6
// using the return_to_a6 function
static ALWAYS_INLINE void jmp_to_with_return_address_in_a6(PFVOID function)  {
    __asm__ volatile (
        "lea.l  1f(pc),a6\t\n" // return address in a6
        "jmp    (%0)\t\n"        // go
        "1:\t\n"
        : // No output
        : "a"(function) // Is it really safe ? How to tell GCC not to use a6 ?
        : "cc" // Registers may be clobbered by the called function but in the context of this startup,
               // we don't care, and we don't even need a6 to be preserved.
        );
}


static ALWAYS_INLINE void return_to_a6(void) {
    __asm__ volatile (
        "jmp    (a6)\t\n"        // go
        : // No output
        : // No parameter
        : // Nothing clobeered
        );
}