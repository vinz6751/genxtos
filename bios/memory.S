/*
 * memory.S - memory initialization
 *
 * Copyright (C) 2001-2025 by Authors:
 *
 * Authors:
 *  MAD  Martin Doering
 *  PES  Petr Stehlik
 *  jfdn Jean-François DEL NERO
 *  VRI  Vincent Rivière
 *  RFB  Roger Burrows
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#include "asmdefs.h"
#include "memory.h"

// ==== Definitions ==========================================================

        .globl  _meminit                 // memory initialization
#if CONF_WITH_ADVANCED_CPU
        .globl  _detect_32bit_address_bus
#endif

// ==== References ===========================================================

        .extern _warm_magic
        .extern _stktop
        .extern _meminit_flags
        .extern amiga_memconf

        // functions for detecting hardware
        .extern _check_read_byte
        .extern _bzero_nobuiltin
        .extern _flush_data_cache
        .extern _invalidate_data_cache

        // ST-RAM settings
        .extern memctrl
        .extern _phystop
        .extern memvalid
        .extern memval2
        .extern memval3

        // TT-RAM settings
        .extern _ramtop
        .extern _ramvalid

        // Overall memory map
        .extern sysvars_start     // start of system variables
        .extern sysvars_end       // end of system variables
        .extern __bss             // start of BSS segment
        .extern __ebss            // end of BSS segment
        .extern __low_stram_start // start of .low_stram section
        .extern __low_stram_end   // end of .low_stram section

// ===========================================================================
// ==== meminit - Reuse or detect RAM settings, then initialize RAM areas ====
// ===========================================================================
_meminit:

/*
 * Exception vectors must point to actual RAM, so exceptions can be used for
 * detection. Furthermore, sp must have been initialized to a valid temporary
 * stack, so we can call subroutines here. This temporary stack may be
 * destroyed by the memory detection mechanism, so we can't store long living
 * data on it.
 */

        /* We will accumulate meminit_flags in this register */
        moveq   #0, d7                  // _meminit_flags = 0

#if CONF_WITH_FALCON_MMU
        /* detect if this is a Falcon by reading $ff8282 register */
        lea     not_a_falcon(pc), a0
        move.l  a0, 0x08.w
        tst.b   0xffff8282.w
        bset    #MEMINIT_BIT_FALCON_MMU, d7

#if !EMUTOS_LIVES_IN_RAM
        /* Initialize and test Falcon MMU ??? */
#ifdef __mcoldfire__
        lea     0xffff8007.w, a0
        bset    #6, (a0)                // seen in TOS 4.04
#else
        bset    #6, 0xffff8007.w        // seen in TOS 4.04
#endif
        jeq     memconf                 // not sure what 6th bit means
        jra     memchk
#endif // !EMUTOS_LIVES_IN_RAM

not_a_falcon:
#endif /* CONF_WITH_FALCON_MMU */

#if CONF_WITH_TT_MMU
        /* Detect if this is a TT.
         * Assume we have a TT MMU if we have a TT Shifter */
        move.l  #not_a_tt,0x08          // bus error vector
        tst.b   0xffff8400.w            // try to access the tt palette registers
        bset    #MEMINIT_BIT_TT_MMU, d7
not_a_tt:
#endif /* CONF_WITH_FALCON_MMU */

// ===========================================================================
// ==== memchk - Check if old memory config can be used ======================
// ===========================================================================
// Inputs:
//   d7.b: _meminit_flags (input/output)
// Outputs:
//   d6.b: old memctrl
//   d5.l: old _phystop
//   d3.l: old _ramvalid (#if CONF_WITH_TTRAM)
//   a3.l: old _ramtop (#if CONF_WITH_TTRAM)
memchk:
        // We must determine the ST-RAM settings in some way.
        // If EmuTOS lives in RAM, always assume that settings of previous OS
        // are correct, and never call memconf.
        // memconf would trash the RAM including EmuTOS itself, anyway.
#if !EMUTOS_LIVES_IN_RAM
        // EmuTOS lives in ROM, we must check if the RAM is valid
#ifdef __mcoldfire__
        move.l  #0x752019f3, d1
        cmp.l   memvalid.w, d1          // magic in memvalid ?
        jne     memconf                 // no -> config memory
        move.l  #0x237698aa, d1
        cmp.l   memval2.w, d1           // magic in memval2 ?
        jne     memconf                 // no -> config memory
        move.l  #0x5555aaaa, d1
        cmp.l   memval3.w, d1           // magic in memval3 ?
        jne     memconf                 // no -> config memory
#else
        cmp.l   #0x752019f3, memvalid.w // magic in memvalid ?
        jne     memconf                 // no -> config memory
        cmp.l   #0x237698aa, memval2.w  // magic in memval2 ?
        jne     memconf                 // no -> config memory
        cmp.l   #0x5555aaaa, memval3.w  // magic in memval3 ? (TOS >= 1.2)
        jne     memconf                 // no -> config memory
#endif // __mcoldfire__
#endif // !EMUTOS_LIVES_IN_RAM

        // The old RAM settings are valid, load them into registers.
        // They will stay in safe place in case we clear the sysvars.
        move.b  memctrl.w, d6
        move.l  _phystop.w, d5

#if CONF_WITH_TTRAM
        move.l  _ramvalid.w, d3
        move.l  _ramtop.w, a3
#endif

// ===========================================================================
// ==== The ST-RAM configuration is valid ====================================
// ===========================================================================
// Inputs:
//   d7.b: _meminit_flags (input/output)
//   d6.b: old memctrl
//   d5.l: old _phystop
//   d3.l: old _ramvalid (#if CONF_WITH_TTRAM)
//   a3.l: old _ramtop (#if CONF_WITH_TTRAM)

#if CONF_WITH_FALCON_MMU
        btst    #MEMINIT_BIT_FALCON_MMU, d7 // is it Falcon MMU?
        jne     memory_reused           // yes -> skip memctrl init
#endif

#if CONF_WITH_ST_MMU
        move.b  d6, 0xffff8001.w        // write old valid config to controller
#endif

memory_reused:
#if CONF_DETECT_FIRST_BOOT_WITHOUT_MEMCONF
        // If _warm_magic == WARM_MAGIC, this is explicitly not a first boot
        lea     _warm_magic.w, a0
        move.l  #0x5741524D, d1         // WARM_MAGIC
        cmp.l   (a0), d1
        jeq     .warm_magic_done        // Indeed, this is not a first boot

        // _warm_magic != WARM_MAGIC: this is a first boot
        bset    #MEMINIT_BIT_FIRST_BOOT, d7

.warm_magic_done:
#endif
        jra     stram_detected

#if !EMUTOS_LIVES_IN_RAM

// ===========================================================================
// ==== memconf - Detect RAM and configure the memory controller =============
// ===========================================================================
// Inputs:
//   d7.b: _meminit_flags (input/output)
// Outputs:
//   d6.b: new value of 0xffff8001 (maybe fake), to put into memctrl.
//   d5.l: detected end of the ST-RAM, to put into _phystop.
// Must be preserved:
//   d3.l: old _ramvalid (#if CONF_WITH_TTRAM)
//   a3.l: old _ramtop (#if CONF_WITH_TTRAM)
memconf:
#if CONF_STRAM_SIZE
        // memconf for hardcoded ST-RAM size
        moveq   #0, d6                  // fake memctrl
        move.l  #CONF_STRAM_SIZE, d5    // _phystop
        // Fall into memconf_done
#elif defined(MACHINE_AMIGA)
        lea     amiga_memconf_ret(pc),a6
        jmp     amiga_memconf
amiga_memconf_ret:
        // Fall into memconf_done
#elif defined(MACHINE_A2560U) || defined(MACHINE_A2560X)
        .extern a2560_memconf
        lea     a2560_memconf_ret(pc),a6
        jmp     a2560_memconf
a2560_memconf_ret:        
#else /* ST/TT/Falcon memconf */

#if CONF_WITH_FALCON_MMU
        btst    #MEMINIT_BIT_FALCON_MMU, d7 // is it Falcon MMU?
        jne     falcmemconf             // yes -> jump to Falcon mem routine
#endif /* CONF_WITH_FALCON_MMU */

#if CONF_WITH_TT_MMU
// ===========================================================================
// ==== memconf for TT =======================================================
// ===========================================================================

        btst    #MEMINIT_BIT_TT_MMU, d7 // is it TT MMU?
        jeq     after_tt_memconf        // no -> skip TT memconf
/*
 * The following configurations are valid for ST-RAM in a TT, listed as
 * total RAM (motherboard RAM + optional daughterboard RAM):
 *  2MB (2MB + 0MB), 4MB (2MB + 2MB), 10MB (2MB + 8MB)
 * [According to the TT rev B MCU functional description, it is also
 * possible to have 8MB (8MB + 0MB) and 10MB (8MB + 2MB) configurations,
 * but I have not seen this.]  When an 8MB board is present, it is always
 * mapped to the first 8MB of memory.
 *
 * Systems with only 2MB boards require a memcntlr value of 0x05; systems
 * with an 8MB board require a memcntlr value of 0x0a.  We detect which
 * size of board is in low memory by assuming an 8MB board, then checking
 * for different memory addresses mapping to the same physical address.
 *
 * rfb (sep/2016)
 */
        move.w  #0x0005,d6          // assume 2MB board
        move.b  #0x0a,0xffff8001.w  // set memory controller for 8MB
        moveq   #0,d0
        move.l  d0,0x1008           // zero out test addresses
        move.l  d0,0x100c
        move.l  #0x25121642,d0      // write test value #1 to an address
        move.l  d0,0x0008           //  that may be mapped into test address #1
        cmp.l   0x1008,d0           // did test address #1 get updated?
        jne     set_8MB             // no, must be an 8MB board
        move.l  #0x14031879,d0      // yes, repeat with test value #2 to be sure
        move.l  d0,0x000c
        cmp.l   0x100c,d0           // did test address #2 get updated?
        jeq     set_2MB             // yes, use the 2MB memcntlr value
set_8MB:
        move.w  #0x000a,d6          // set 8MB memcntlr value
set_2MB:
        move.b  d6,0xffff8001.w     // update memcntlr

/*
 * We now need to size the actual memory present.  We test memory,
 * starting at 128K and increasing by 128K each time.  For each area of
 * memory, we start at the current address, and work downwards, storing
 * a series of values; we then verify that the values have been stored
 * correctly.  An invalid value (or a bus error when we try to access a
 * memory location) causes us to stop and size the memory accordingly.
 */
        move.l  #tt_stramsize_done,0x08 // bus error means end of ram
        move.w  #0xf769,d4          // d4.w = difference between adjacent stored values
        move.l  #0x20000,d7         // d7 = address of first memory test zone
                                    //  & gap between memory test zones
        movea.l d7,a0               // a0 = address of current test zone
next_zone:
        movea.l a0,a1               // a1 -> memory being tested
        move.w  d0,d5               // d5.w = test value
        moveq   #32,d1              // d1.w = loop count-1
store_mem:
        move.w  d5,-(a1)            // store a value
        add.w   d4,d5               // update the value for next time
        dbra    d1,store_mem        // rinse & repeat
                                // now check the values we stored
        movea.l a0,a1               // a1 -> test area
        moveq   #32,d1              // loop count
check_mem:
        cmp.w   -(a1),d0
        jne     tt_stramsize_done   // miscompare, ram invalid, exit
        clr.w   (a1)                // else zero it out
        add.w   d4,d0               // update for next compare
        dbra    d1,check_mem        // until we're done
        adda.l  d7,a0               // point to next test area
        jra     next_zone

tt_stramsize_done:                  // bus error or bad ram
        suba.l  d7,a0               // (a0 -> first zone that failed)
        move.l  a0,d5               // d5 = st ram size

        // While we are at it, initialize the TT-RAM hardware.
        // Actual size of TT-RAM will be detected later in detect_ttram_size().
        // We must set the TT-RAM refresh rate before we use it.  The
        // rate is set by writing a "don't care" value to an address in
        // the range 0xffd00000-0xffd000ff; the low-order byte of the
        // address sets the refresh rate (specifically the number of
        // system clock cycles between each refresh request).  TT TOS
        // uses the value 0xe0; so do we.
        // NOTE: whether we actually have TT-RAM or not, setting the
        // value always causes a bus error, so we need to handle it.
        // NOTE2: We use the "move.w #0" instruction because "clr.w" oddly
        // generates a useless read cycle on plain 68000. That might matter
        // regarding to this very specific refresh rate function. Even if TT
        // computers are supposed to be equipped with 68030 which is not subject
        // to this 68000-specific oddity, we stay on the safe side.
        move.l  #refresh_done,0x08      // bus error vector
        move.w  #0,0xffd000e0           // Set TT-RAM refresh rate
refresh_done:

        jra     memconf_done
after_tt_memconf:
#endif /* CONF_WITH_TT_MMU */

#if CONF_WITH_ST_MMU

// ===========================================================================
// ==== memconf for ST =======================================================
// ===========================================================================
/*
 * At first the memory configuration is set to 4 Mb. If the memory
 * configuration is not really 2 * 2 mb, the written test values will
 * repeat in memory. The trick is to find these repeated values and
 * set the memory config accordingly. No real memory check is done,
 * because in emulators it should be always ok - hopefully!
 *
 * 24/05/10 - jfdn
 * Note: To be reliable on the real hardware, the pattern test algorithm
 * is now the same used in the TOS 1.6 ROM.
 * Using of only 1 word pattern test is not reliable since without SIMM
 * the memory bus are floating and may present the good pattern value.
 * That's why a 200 bytes pattern test are used in the TOS 1.6 ROM to avoid this issue.
 */
        move.b  #0xa,0xffff8001.w       // set hw to 2 banks by 2 mb (reg 0xffff8001 - memorycontroller)

/* init the test pattern in bank 0 and 1 */
        movea.w #0x000008,a0            // bank 0
        lea     0x200008,a1             // bank 1
        moveq   #0,d0
init_pattern_loop:
        move.w  d0,(a0)+                // bank 0
        move.w  d0,(a1)+                // bank 1
        add.w   #0xfa54,d0              // next pattern data
        cmpa.l  #0x200,a0               // 200 word done?
        jne     init_pattern_loop

/* In this part:
 * D4 = Actual bank memory size in byte
 * D5 = total memory size (later phystop)
 * D6 = value to the MMU config register (reg 0xffff8001) at the end
 * A0 = pattern test offset (0x8,0x40008,0x80008)
 * D1 = pattern test bank base (0x200000 ,0x000000)
 * A5 = pattern test subroutine return address.
 */
        move.l  #0x200000,d1            // we begin with the bank 1
        moveq   #0,d5                   // total memory (later phystop)
        moveq   #0,d6                   // MMU setting set to 0

        /* test the presence of a 128k bank config */
test_128k:
        lsr.w   #2,d6                   // switch bank in configuration byte
        move.l  #0x20000,d4             // 128 KB bank memory size
        movea.l #0x40008,a0             // 128k test for STE
        lea     test_128k_ST,a5         // return point of test pattern
        jra     check_test_pattern

test_128k_ST:
        jeq     bank_test_end           // There is a memory loop in the previous test ! -> D6=0x0 - 128k bank
        movea.w #0x208,a0               // 128k test for ST
        lea     test_512k,a5            // return point of test pattern
        jra     check_test_pattern

        /* test the presence of a 512k bank config */
test_512k:
        jeq     bank_test_end           // There is a memory loop in the previous test ! -> D6=0x0 - 128k bank

        move.l  #0x80000,d4             // 512 KB bank memory size
        movea.l #0x80008,a0             // 512k test for STE
        lea     test_512k_ST,a5         // return point of test pattern
        jra     check_test_pattern

test_512k_ST:
        jeq     bank_test_end_512k      // There is a memory loop in the previous test -> D6=0x4 - 512K bank
        movea.w #0x408,a0               // 512k test for ST
        lea     test_2m,a5              // return point of test pattern
        jra     check_test_pattern

        /* test the presence of a 2M bank config */
test_2m:
        jeq     bank_test_end_512k      // There is a memory loop in the previous test -> D6=0x4 - 512K bank

        /* This is the last test. In previous tests (256k&512k) we didn't find any memory loop.
           Here we are testing if there a 2Mb simm (full bank memory size) or nothing. */

        movea.w #0x008,a0               // 0kb/2Mb test for ST & STE
        lea     last_return,a5          // return point of test pattern
        moveq   #0x0,d4                 // 0kb bank memory size
        jra     check_test_pattern

last_return:
        jne     bank_test_end           // Pattern not found!  -> D6=0x0 - NO RAM
        move.l  #0x200000,d4            // Pattern found!      -> D6=0x6 - 2m

        addq.w  #4,d6

bank_test_end_512k:
        addq.w  #4,d6

bank_test_end:
        add.l   d4,d5                   // compute total memory size
        sub.l   #0x200000,d1            // next bank 0
        jeq     test_128k               // already done ?   -> no start the tests for bank 0
                                        // in this case this test is the end.

        // Set the final MMU config
        move.b  d6,0xffff8001.w         // set hw memory config (reg 0xffff8001 - memorycontroller)

        jra     detect_extra_stram

/*
 * check_test_pattern "subroutine":
 * D1= bank base   (Bank0: 0x000000 Bank1: 0x200000)
 * A0= test offset (0x8,0x40008,0x80008)
 * A5= return point
 * if Z=1 -> pattern found !
 * if Z=0 -> pattern not found !
 */
check_test_pattern:
        adda.l  d1,a0                   // compute starting address
        moveq   #0,d0                   // first pattern word
        lea     0x1f8(a0),a1            // end address
check_test_pattern_loop:
        cmp.w   (a0)+,d0                // is the data valid ?
        jne     end_check_test_pattern
        add.w   #0xfa54,d0              // next pattern word to test
        cmpa.l  a0,a1                   // at the end of the pattern ?
        jne     check_test_pattern_loop
end_check_test_pattern:
        jmp     (a5)                    // return

detect_extra_stram:
        /* Now detect if there is some extra ST-RAM installed */
        cmpi.l  #4*1024*1024,d5         // we should already have 4 MB
        jne     detect_extra_stram_end

        lea     5*1024*1024-1,a0        // start at the end of 5 MB
loop_extra_stram:
        pea     (a0)
        jsr     _check_read_byte        // test the last byte
        addq.l  #4,sp
        tst.w   d0                      // readable ?
        jeq     detect_extra_stram_end  // no, stop here

        move.l  #1*1024*1024,d0         // one more megabyte
        add.l   d0,d5                   // to the current amount of ram

        cmp.l   #14*1024*1024,d5        // check for 14 MB maximum
        jhs     detect_extra_stram_end

        add.l   d0,a0                   // and to next address to check
        jra     loop_extra_stram
detect_extra_stram_end:

        /* Some emulators do not emulate the behaviour of memory banks
         * so we end up here with a bad value in d5 ... let's do a
         * last sanity check whether the memory is really usable: */
        move.l  #0xff0055aa,d0          // Test pattern
mem_sanity_loop:
        move.l  d5,a0
        subq.l  #4,a0
        move.l  d0,(a0)                 // Write to memory
        cmp.l   (a0),d0                 // Was memory really writable?
        jeq     mem_sanity_done
        sub.l   #512*1024,d5            // Decrease memory size
        jra     mem_sanity_loop
mem_sanity_done:
        jra     memconf_done

#endif /* CONF_WITH_ST_MMU */

#if CONF_WITH_FALCON_MMU

// ===========================================================================
// ==== memconf for Falcon ===================================================
// ===========================================================================
falcmemconf:
        /* find out the memory size by checking the d0 = $ff8006 value */
#ifdef __mcoldfire__
        moveq   #0, d0
        move.w  0xffff8006.w, d0        // read the Falcon MMU register
        lsr.l   #8,d0
        lsr.l   #1,d0
        move.l  d0,d1
        and.l   #1,d1
        lsr.l   #2,d0
        and.l   #6,d0
        or.l    d0,d1
#else
        move.w  0xffff8006.w, d0        // read the Falcon MMU register
        lsr.w   #8,d0
        lsr.b   #1,d0
        move.b  d0,d1
        and.b   #1,d1
        lsr.b   #2,d0
        and.b   #6,d0
        or.b    d0,d1
#endif
        move.l  #0x00080000,d0
        asl.l   d1,d0
        cmp.b   #5,d1
        jne     less16
        move.l  #0x00E00000,d0
less16:
        move.l  d0, d5                  // this is phystop - the end of RAM
        moveq   #0, d6                  // fake memctrl with Falcon MMU

        // Fall into memconf_done

#endif /* CONF_WITH_FALCON_MMU */

#endif /* ST/TT/Falcon memconf */

memconf_done:
        // Since the memory was not valid, this is a first boot
        bset    #MEMINIT_BIT_FIRST_BOOT, d7
#if CONF_WITH_TTRAM
        // Since the ST-RAM was invalid, TT-RAM settings were also invalid
        moveq   #0, d3                  // _ramvalid
#endif

#endif // !EMUTOS_LIVES_IN_RAM

// ===========================================================================
// ==== End of ST-RAM detection ==============================================
// ===========================================================================
/*
 * At this point:
 * - The size of the ST-RAM has been detected
 * - The MMU has been properly initalized
 * - The ST-RAM is ready to be used
 *
 * d7.b = _meminit_flags
 * d5.l = new phystop
 * d6.b = new memctrl
 * d3.l = old _ramvalid (#if CONF_WITH_TTRAM)
 * a3.l = old _ramtop (#if CONF_WITH_TTRAM)
 */
stram_detected:
        // The previous temporary stack pointer may have moved
        // during early detections, so we reinitialize it now.
        // After that, normal stack usage is allowed.
        lea     _stktop, sp

        btst    #MEMINIT_BIT_FIRST_BOOT, d7 // first boot ?
        jeq     any_boot                // no: skip first boot initialization

// ===========================================================================
// ==== This part is executed only on first boot =============================
// ===========================================================================

        /* Clear the sysvars */
        move.l  #sysvars_start.w, a0
        move.l  #sysvars_end, a1
        jbsr    call_bzero_a0_a1

        /* Store the ST-RAM parameters in the ST-RAM itself */
        move.b  d6, memctrl.w           // set copy of hw memory config
        move.l  d5, _phystop.w          // highest address as phystop

        /* Make ST-RAM configuration valid */
#ifdef __mcoldfire__
        lea     memvalid.w, a0
        move.l  #0x752019f3, (a0)       // set memvalid to ok
        lea     memval2.w, a0
        move.l  #0x237698aa, (a0)       // set memval2 to ok
        lea     memval3.w, a0
        move.l  #0x5555aaaa, (a0)       // set memval3 to ok
#else
        move.l  #0x752019f3, memvalid.w // set memvalid to ok
        move.l  #0x237698aa, memval2.w  // set memval2 to ok
        move.l  #0x5555aaaa, memval3.w  // set memval3 to ok
#endif

#if CONF_DETECT_FIRST_BOOT_WITHOUT_MEMCONF
        lea     _warm_magic.w, a0
        move.l  #0x5741524D, d1         // WARM_MAGIC
        move.l  d1, (a0)                // Next boot will not be a first boot
#endif

#if CONF_WITH_TTRAM
        // After clearing sysvars, restore old TT-RAM settings.
        // They will be used later for TT-RAM initialization.
        move.l  a3, _ramtop.w           // set ramtop to size of found memory
        move.l  d3, _ramvalid.w
#endif

// ===========================================================================
// ==== This part is executed on any boot (first boot or reset) ==============
// ===========================================================================
 .globl any_boot
any_boot:
        // Clear the BSS segment.
        // Our stack is explicitly set outside the BSS, so this is safe:
        // bzero() will be able to return.
        move.l  #__bss, a0
        move.l  #__ebss, a1
        jbsr    call_bzero_a0_a1

        // Also clear the variables located in the .low_stram section.
        move.l  #__low_stram_start, a0
        move.l  #__low_stram_end, a1
        jbsr    call_bzero_a0_a1

        // Save the meminit_flags into BSS
        move.b  d7, _meminit_flags

        .globl _startup_stage2
        jmp _startup_stage2 // config done -> return


// ===========================================================================
// ==== Below are utility functions ==========================================
// ===========================================================================

// Clear memory from a0 to a1
call_bzero_a0_a1:
        sub.l   a0,a1
        move.l  a1,-(sp)    // size
        move.l  a0,-(sp)    // start
        jsr     _bzero_nobuiltin
        addq.l  #8,sp
        rts

#if CONF_WITH_ADVANCED_CPU

// Detect 32-bit address bus
// Returns:
//   d0.w = 1 if 32-bit, or 0 if 24-bit
// Clobbers:
//   - Exception Vectors
_detect_32bit_address_bus:
        movem.l d2/a2-a4,-(sp)          // Save registers
        move.l  sp,a2                   // Save sp before exception
        move.l  #bus_detected,0x08      // Bus Error will just return

        lea     0x0000000c.w,a3         // Test address is Address Error vector
        lea     0x0100000c,a4           // 32-bit mirror address

        move.l  #0x12345678,d2          // First test value
        move.l  d2,(a3)                 // Write test value to 24-bit address
        jbsr    sync_data_cache         // Mandatory if data cache is enabled
        // If we are on Atari hardware with 32-bit address bus but without
        // TT-RAM, then the instruction below will cause a Bus Error.
        cmp.l   (a4),d2                 // Did value appear on 32-bit mirror?
        jne     bus_detected            // No, we are on 32-bit bus

        // Test again in case where the mirror already had the test value
        not.l   d2                      // Second test value
        move.l  d2,(a3)                 // Write test value to 24-bit address
        jbsr    sync_data_cache         // Mandatory if data cache is enabled
        cmp.l   (a4),d2                 // Did value appear on 32-bit mirror?
        jne     bus_detected            // No, we have a 32-bit address bus

        moveq   #0,d0                   // Yes, we have a 24-bit address bus
        jra     end_detect_bus

bus_detected:
        moveq   #1,d0                   // We have a 32-bit by address bus

end_detect_bus:
        move.l  a2,sp                   // Restore sp in case of exception
        movem.l (sp)+,d2/a2-a4          // Restore registers
        rts

sync_data_cache:
        pea     4.w                     // Size of test value
        move.l  a3,-(sp)                // Test address
        jbsr    _flush_data_cache       // Push to RAM
        addq.l  #8,sp

        pea     4.w                     // Size of test value
        move.l  a4,-(sp)                // Mirror address
        jbsr    _invalidate_data_cache  // Remove value from cache
        addq.l  #8,sp

        rts

#endif

#if CONF_WITH_MEMORY_TEST

// ===========================================================================
// ==== utility functions for memory test ====================================
// ===========================================================================

        .globl  _memtest_verify
        .globl  _memtest_rotate_verify

//
// BOOL memtest_verify(ULONG *start, ULONG value, LONG length)
//
_memtest_verify:
        movea.l 4(sp),a0                // a0-> start
        move.l  8(sp),d0                // d0 = value to check
        move.l  12(sp),d1               // d1 = length (bytes)
        lsr.l   #2,d1                   // d1 = length (longs)
        subq.l  #1,d1                   // for dbne
vloop:
        cmp.l   (a0)+,d0                // ok?
#ifdef __mcoldfire__
        jne     verror                  // no, branch to error
        subq.l  #1,d1
        jpl     vloop
#else
        dbne    d1,vloop
        jne     verror
        subi.l  #0x10000,d1             // handle long value for count
        jhi     vloop
#endif
        moveq   #1,d0                   // all ok, return TRUE
        rts
verror:
        moveq   #0,d0                   // error, return FALSE
        rts

//
// BOOL memtest_rotate_verify(ULONG *start, LONG length)
//
_memtest_rotate_verify:
        movea.l 4(sp),a0                // a0-> start
        moveq.l #1,d0                   // d0 = value to store
        move.l  8(sp),d1                // d1 = length (bytes)
        lsr.l   #2,d1                   // d1 = length (longs)
        subq.l  #1,d1                   // for dbne
rloop:
#ifdef __mcoldfire__
        lsl.l   #1,d0                   // rotate bit
        jcc     rnocarry
        moveq   #1,d0
rnocarry:
#else
        rol.l   #1,d0
#endif
        cmp.l   (a0)+,d0                // store
#ifdef __mcoldfire__
        jne     rerror
        subq.l  #1,d1
        jpl     rloop
#else
        dbne    d1,rloop
        jne     rerror
        subi.l  #0x10000,d1             // handle long value for count
        jhi     rloop
#endif
        moveq   #1,d0                   // all ok, return TRUE
        rts
rerror:
        moveq   #0,d0                   // error, return FALSE
        rts

#endif
