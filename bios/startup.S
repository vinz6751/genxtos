/*
 * startup.S - EmuTOS startup module
 *
 * Copyright (C) 2001-2025 The EmuTOS development team
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

 /*
  * Startup consists into jumping after OS header, then jumping
  * above EmuTOS extended OS header (specifying machine type, CPU type etc)
  */


#include "asmdefs.h"
#include "../obj/header.h"

// ==== Definitions ==========================================================

        .globl  _os_header
        .globl  _main         // OS entry point


// ==== References ===========================================================

        // TOS System variables
        .extern _stktop

        // BIOS C part
        .extern _biosmain     // BIOS C entry point

        // Overall memory map
        .extern __etext       // end of text section
        .extern __edata       // end of data section
        .extern __endvdibss   // end of VDI BSS

        .extern _root
        .extern _shifty
        .extern _run
        .extern _ui_mupb

// ===========================================================================
// ==== TEXT segment (TOS image) =============================================
// ===========================================================================

        .text

#if DIAGNOSTIC_CARTRIDGE
    .dc.l   cart_magic
#endif

#if defined(MACHINE_AMIGA) && !EMUTOS_LIVES_IN_RAM
    .dc.w   0x1111      // 256 KB ROM
    .dc.w   0x4ef9      // jmp
    .dc.l   amiga_main  // pc at power on or after physical reset

    /* Some softkick programs (ex: BlizKick) requires the entry point to be here */
    .org    0xd0
    reset
amiga_main:
    // fall through os_entry
#endif

#if (defined(MACHINE_A2560U) || defined(MACHINE_A2560X) || defined(MACHINE_A2560M)) && !EMUTOS_LIVES_IN_RAM
#if 1
    // This is disabled because running EmuTOS from flash bank0 (0xe00000) can lead to problems because the A2560U
    // crashes if we try to run user code from there. This is an issue e.g. for running EmuCON from flash.
    // So the only option is to install the OS in from flash bank1 (0xf00000), but then the boot process must be
    // different:
    // As Gavin copies the first 64k of flash (ie of flash bank 0), these 64Kb don't belong to the EmuTOS image,
    // and whatever is in flash bank 0, must take care of calling the EmuTOS startup.
        // 68000 reset vectors. Gavin will copy the first 64k of flash to the system ram then reset the proc.
        // All we need to do is to provide a reset vector that jumps to the flash.
        .dc.l 0x1000       // Stack: rubbish, should not be used anyway as the OS should setup its own stack
        .dc.l _os_header  // jump to the OS
#endif
#endif

/*
 * OSHEADER
 */

_os_header:
os_entry:    bra.s   _main       // branch to _main
os_version:  .dc.w   TOS_VERSION // TOS version
reseth:      .dc.l   _main       // pointer to reset handler
// The 3-long structure here is actually the default GEM_MUPB.
// It is supposed to be used when os_magic does not point to a valid GEM_MUPB.
// In EmuTOS, this never happens, so os_end and os_res1 below are never used.
// Note that the first field here has a different meaning from gm_magic.
os_beg:      .dc.l   _os_header  // base of OS = _sysbase
os_end:      .dc.l   __endvdibss // end of VDI BSS
os_res1:     .dc.l   _main       // reserved
os_magic:    .dc.l   _ui_mupb    // pointer to main UI's GEM_MUPB
os_date:     .dc.l   OS_DATE     // Date of system build
os_conf:     // Flag for PAL version + country
#if CONF_MULTILANG
    .dc.w   OS_CONF_MULTILANG
#else
    .dc.w   (OS_COUNTRY << 1) + OS_PAL
#endif
os_dosdate:  .dc.w   OS_DOSDATE  // Date of system build in GEMDOS format
os_root:     .dc.l   _root       // Pointer to the GEMDOS mem pool
os_kbshift:  .dc.l   _shifty     // Pointer to the keyboard shift keys states
os_run:      .dc.l   _run        // Pointer to a pointer to the actual basepage
os_dummy:    .ascii  "ETOS"      // _main should start at offset 0x30, shouldn't it?


_main:
        bra.w   _realmain   // MUST be 4 bytes long


#include "os_xheader.S"

/*
 * Get in supervisor mode and reset all Hardware
 *
 * The following considerations have been taken into account:
 * 1. for unknown reasons, real Falcons need an access to location $ffff8006
 *    before and after the reset instruction
 * 2. on some non-Falcon systems (some STs and some or all TTs), accessing
 *    $ffff8006 causes a bus error; therefore the bus error vector must be
 *    intercepted prior to #1
 * 3. since non-68000 systems use the VBR to point to the start of the exception
 *    vectors, the VBR must be zeroed prior to #2 via MOVEC
 * 4. in order to run the same code on 68000 systems (which don't have MOVEC),
 *    the illegal instruction vector must be intercepted prior to #3.  for this,
 *    it doesn't matter if the VBR is non-zero because, if it is, the MOVEC
 *    instruction must be legal so the trap won't get taken ...
 */

_realmain:
        // Disable the interrupts
        // this is useless on cold boot, but it matters for software reset
        move    #0x2700,sr

        // Initialize an early stack to a valid write address.
        // We use early exceptions for 2 reasons:
        // - detect CPU type and features with illegal instructions
        // - check for available hardware with bus errors
        // When an exception occurs, the CPU will to store the
        // return address and sr on this stack.
        // Since the RAM has not yet been initialized, this stack may not
        // point to actual memory. It is only required that the CPU will
        // not cause a second exception when writing to that address.
        // The exception handlers use a register as return address.
        // It must never be assumed that this early stack can be used to store any data.
        // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
        // [VB] THAT IS ASSUMED NOW. If the RAM is not minimally functional, how could we 
        // even store vectors we use during CPU detection ?
        
        // We take this opportunity to set the stack pointer to the
        // address which will become our BIOS stack after memconf.
        lea     _stktop,sp

        .globl _startup
        jbsr    _startup

        .globl _startup_stage2
_startup_stage2:
        lea     _stktop,sp      // Reset the stack to the top
        jmp     _biosmain
