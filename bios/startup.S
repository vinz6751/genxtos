/*
 * startup.S - EmuTOS startup module
 *
 * Copyright (C) 2001-2021 The EmuTOS development team
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

 /*
  * Startup consists into jumping after OS header, then jumping
  * above EmuTOS extended OS header (specifying machine type, CPU type etc)
  */

        .GLOBL _channel_A_logger_init
        .GLOBL _channel_A_cls
        .GLOBL _channel_A_set_cursor_pos

#include "asmdefs.h"
#include "../obj/header.h"

// ==== Defines ==============================================================

        .equ    cart_base, 0x00fa0000  // Base of ROM port cartridge
        .equ    cart_magic, 0xfa52235f // diagnostic cartridge magic number

// ==== Definitions ==========================================================

        .globl  _os_header
        .globl  _main         // OS entry point
#if CONF_WITH_CARTRIDGE
        .globl  _run_cartridge_applications
#endif

// ==== References ===========================================================

        // TOS System variables
        .extern resvalid
        .extern resvector
        .extern _stktop

        // Memory
        .extern meminit       // initialize the memory and its controller

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
        .extern amiga_startup

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

#if (defined(MACHINE_A2560U) || defined(MACHINE_A2560X)) && !EMUTOS_LIVES_IN_RAM
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
os_entry:
    bra.s   _main       // branch to _main
os_version:
    .dc.w   TOS_VERSION // TOS version
reseth:
    .dc.l   _main       // pointer to reset handler
// The 3-long structure here is actually the default GEM_MUPB.
// It is supposed to be used when os_magic does not point to a valid GEM_MUPB.
// In EmuTOS, this never happens, so os_end and os_res1 below are never used.
// Note that the first field here has a different meaning from gm_magic.
os_beg:
    .dc.l   _os_header  // base of OS = _sysbase
os_end:
    .dc.l   __endvdibss // end of VDI BSS
os_res1:
    .dc.l   _main       // reserved
os_magic:
    .dc.l   _ui_mupb    // pointer to main UI's GEM_MUPB
os_date:
    .dc.l   OS_DATE     // Date of system build
os_conf:                // Flag for PAL version + country
#if CONF_MULTILANG
    .dc.w   OS_CONF_MULTILANG
#else
    .dc.w   (OS_COUNTRY << 1) + OS_PAL
#endif
os_dosdate:
    .dc.w   OS_DOSDATE  // Date of system build in GEMDOS format
os_root:
    .dc.l   _root       // Pointer to the GEMDOS mem pool
os_kbshift:
    .dc.l   _shifty     // Pointer to the keyboard shift keys states
os_run:
    .dc.l   _run        // Pointer to a pointer to the actual basepage
os_dummy:
    .ascii  "ETOS"      // _main should start at offset 0x30, shouldn't it?

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
        .global _a2560_beeper
_realmain:
        // disable the interrupts
        // this is useless on cold boot,
        // but it matters for software reset
        move    #0x2700,sr
        move.l #16,0xfec00000
#ifdef MACHINE_A2560X
        move.l #19,0xfec00000 // buzzer, all leds
        move.l #0,0xfec80008 // reset border color
        move.l #0xffffffff,0xfec40008
        move.l #0xffffffff,0xfec80008

        
        move.l #0xff008800,0xfec80008 // border color for debug 
#endif

#if CONF_WITH_TT_MMU
        // TT TOS does the following even before reset; it is
        // apparently required for correct floppy functioning
        // when EmuTOS is ROM-resident (report by Ingo Uhlemann).
        // It _should_ be harmless on all Atari systems; it has
        // been tested (via emutos.prg) on ST/TT/Falcon.
        move.w  #0x0100,0xffff8606.w    // toggle the ACSI DMA write bit
        move.w  #0x0000,0xffff8606.w
#endif

#ifdef MACHINE_AMIGA
        // It is *mandatory* to call this as soon as possible,
        // to do early initialization of the Amiga hardware.
        // On cold boot, address 0 points to the ROM.
        // After that, it will point to the RAM, as expected.
        lea     amiga_startup_ret,a6
        jmp     amiga_startup
amiga_startup_ret:
#endif

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
# ifdef __mcoldfire__
        move.b  0xffff8001.w, d0
        cmpi.b  #0xc, d0
        blt.s   st_mmu_config
        move.b  #0x5, d0
        move.b  d0, 0xffff8001.w
# else
        cmpi.b  #0xc, 0xffff8001.w
        blt.s   valid_st_mmu_config
        move.b  #0x5, 0xffff8001.w
# endif

valid_st_mmu_config: // ST MMU has a valid config
#endif

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
        // It must never be assumed that this early stack can be used
        // to store any data.
        // We take this opportunity to set the stack pointer to the
        // address which will become our BIOS stack after memconf.
        lea     _stktop,sp

// Note: LisaEm doesn't properly implement movec/pmove illegal instructions,
// so we just avoid them on MACHINE_LISA.



#if !defined(__mcoldfire__) && !defined(MACHINE_LISA)
        // first, reset the VBR
        // it may have been modified before emutos-ram or soft reset
        move.l  #vbr_ok,16     // must be 68000 if we get illegal instrn error
        moveq   #0,d0
        MOVEC_D0_VBR           // initialise VBR to zero
vbr_ok:
#endif

#if CONF_WITH_RESET && !defined(MACHINE_AMIGA)
        // now, reset the external hardware

        // we don't need to call reset on Amiga
        // because either this is a cold boot
        // or reset has been called just before amiga_main

# if CONF_ATARI_HARDWARE
        // on Falcon, we need special care around the reset instruction
        move.l  #not_falcon,8  // can't be Falcon if we get bus error

        // special Falcon reset
        tst.w   0xffff8006.w   // magic: must access this before & after the reset
        reset                  // reset all hardware
        tst.w   0xffff8006.w   // remaining magic
        jra     main_after_reset_instruction
# endif /* CONF_ATARI_HARDWARE */

not_falcon:
        // we are not on Falcon, a simple reset is enough
        reset
main_after_reset_instruction:
#endif /* CONF_WITH_RESET && !defined(MACHINE_AMIGA) */

#ifdef __mcoldfire__
        lea     coldfire_startup_ret,a6
        jmp     coldfire_startup
coldfire_startup_ret:
#endif



#if CONF_WITH_CARTRIDGE
/*
 * Check for diagnostic cartridge
 */

#ifdef __mcoldfire__
        move.l  cart_base, d0
        cmp.l   #cart_magic, d0         // magic - is cartridge present?
#else
        cmp.l   #cart_magic, cart_base  // magic - is cartridge present?
#endif
        jne     nodiag                  // no -> go on

#if EMUTOS_LIVES_IN_RAM
        move.l  cart_base+0x30, d0
        cmp.l   #0x45544f53, d0         // 'ETOS' - is this an EmuTOS cartridge?
        jeq     nodiag                  // -> don't boot it from a RAM version
#endif

        lea     nodiag(pc), a6          // save return address
        jmp     cart_base+4             // execute diagnostic cartridge
nodiag:
#endif /* CONF_WITH_CARTRIDGE */

/*
 * disable the instruction/data caches
 */
#if !defined(__mcoldfire__) && !defined(MACHINE_LISA)
        move.l  #cache_done,0x10   // must be 68000 if we get illegal instrn error
        move.l  #0x00000808,d0     // clear & disable instruction/data caches on
        MOVEC_D0_CACR              //  68020-60 (even though bit usage differs!)
cache_done:
#endif

/*
 * disable the MMU
 */
#if !defined(__mcoldfire__) && !defined(MACHINE_LISA)
        move.l  #not_68030,0x2c    // PMOVE is lineF on non-68030 systems
        lea     zero,a0
        PMOVE_A0_TTR0              // first we initialise the TTRs (ACRs on a 68ec030)
        PMOVE_A0_TTR1
        move.l  #mmu_done,0x2c     // since PMOVE (A0),TC doesn't exist on a 68ec030,
        PMOVE_A0_TC                //  we're done if we get a lineF exception ...
        jra     mmu_done
not_68030:
        move.l  #mmu_done,0x10     // if a MOVEC causes an exception, we're done
        moveq   #0,d0
        MOVEC_D0_ITT0              // first we initialise the TTRs (ACRs on a 68ec040)
        MOVEC_D0_ITT1
        MOVEC_D0_DTT0
        MOVEC_D0_DTT1
        MOVEC_D0_TC                // disable translation on 68040-60 (will error out
                                   //  on a 68ec040)
mmu_done:
#endif



/*
 * Check, if we should jump to a reset vector after warmboot
 */

resetvec:
#ifdef __mcoldfire__
       move.l   resvalid.w, d0
       cmpi.l   #0x31415926, d0         // Jump to resetvector?
#else
       cmpi.l   #0x31415926, resvalid.w // Jump to resetvector?
#endif
       jne      noreset                 // No --> noreset
       move.l   resvector.w, d0         // Yes: old resvec to d0
       jeq      noreset                 // Address null? Yes --> noreset
       btst     #0, d0                  // Address odd ?
       jne      noreset                 // Yes --> noreset
       movea.l  d0, a0                  // resvec
       lea      resetvec(pc), a6        // save return address
       // Note: the return address intentionally points to the resvalid test.
       // So the reset routine must explicitly clear resvalid before returning
       // to a6, otherwise there will be an infinite loop.
       jmp      (a0)                    // jump to resvec
noreset:

/*
 * Detect memory configuration memory
 */

        .globl  memdone         // return to, if memory config done
        jmp     meminit         // do/test memory config and clear it (needs sp)
memdone:

/*
 * From now, the stack is allowed, and C code can be used.
 * The BIOS startup goes on in bios.c
 */

        move.l #0xff008800,0xfec80008 // border color for debug 


        jmp     _biosmain

#if CONF_WITH_CARTRIDGE
/*
 * void run_cartridge_applications(WORD typebit);
 *
 * Run all cartridge applications of the specified type.
 *
 * typebit: application type bit number which must be set to 1 to match
 */
_run_cartridge_applications:
        move.w  4(sp),d0                // application bit number
        lea     cart_base, a0
#ifdef __mcoldfire__
        move.l  #0xABCDEF42, d1
        cmp.l   (a0)+, d1               // is cartridge present?
#else
        cmp.l   #0xABCDEF42, (a0)+      // is cartridge present?
#endif
        jne     cartover                // no -> cartover
testtype:
        btst    d0, 4(a0)               // What type?
        jeq     nextapp

#ifdef __mcoldfire__
        lea     -60(sp), sp
        movem.l d0-d7/a0-a6, (sp)       // save registers
#else
        movem.l d0-d7/a0-a6, -(sp)      // save registers
#endif
        move.l  4(a0),d0                // d0 = address of cartridge init
        andi.l  #0x00ffffff,d0          // make it 24-bit clean
        movea.l d0,a0
        jsr     (a0)                    // execute app in cartridge
#ifdef __mcoldfire__
        movem.l (sp), d0-d7/a0-a6       // restore registers
        lea     60(sp), sp
#else
        movem.l (sp)+, d0-d7/a0-a6      // restore registers
#endif
nextapp:
        tst.l   (a0)                    // another application?
        move.l  (a0), a0
        jne     testtype
cartover:
        rts
#endif /* CONF_WITH_CARTRIDGE */

// ===========================================================================
// ==== Read-only data =======================================================
// ===========================================================================
        SECTION_RODATA

        .even   // Mandatory in ELF section .rodata
zero:   .dc.l   0
