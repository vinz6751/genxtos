/*
 * processor.S - Detect and set CPU and FPU type
 *
 * Copyright (C) 2002-2025 The EmuTOS development team
 * Copyright (C) 1999, 2002 by Authors
 *
 * Authors:
 *      Jörg Westheide <joerg_westheide@su.maus.de>
 *      Draco
 * LVL  Laurent Vogel
 * MAD  Martin Doering
 *      Norman Feske
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#include "asmdefs.h"

/* References */

        .globl  _processor_init
        .globl  _invalidate_instruction_cache
        .globl  _instruction_cache_kludge
        .globl  _flush_data_cache
        .globl  _invalidate_data_cache
        .globl  _mcpu
        .globl  _fputype
#if CONF_WITH_APOLLO_68080
        .globl  _is_apollo_68080
#endif
#if CONF_WITH_CACHE_CONTROL
        .globl  _cache_exists
        .globl  _set_cache
#endif

        .extern _longframe              // If not 0, use long stack frames
        .extern _setup_68030_pmmu
#if CONF_WITH_68030_PMMU
        .extern _pmmutree
#endif

        .text

#ifndef __mcoldfire__

/*
 * _detect_cpu - CPU detection
 *
 * Returns a long word indicating the type of CPU.  The low-order word
 * contains the generic type, and the high-order word indicates the
 * subtype.  Current returned values are as follows:
 *   low-order    high-order    CPU type
 *       0             0        68000
 *      10             0        68010
 *      20             0        68020
 *      30             0        68030
 *      30             1        68ec030
 *      40             0        68040
 *      60             0        68060
 */

_detect_cpu:
#if CONF_WITH_ADVANCED_CPU
        movem.l  d2/a2-a3,-(sp)

// intercept possible exceptions

        move.l  (0x10).w,a1     // save exception vectors: illegal instruction
        move.l  (0x2c).w,a2     //   line F
        move.l  (0xf4).w,a3     //     unimplemented instruction
        lea     exit.w(PC),a0
        move.l  a0,(0x10).w     // replace vectors
        move.l  a0,(0x2c).w
        move.l  a0,(0xf4).w
        move.l  sp,a0           // save the ssp
        nop                     // eventually flush pipelines

// on 68000 we try out a `move from CCR'.

        moveq   #0,d0           // assume 68000
        MOVEW_CCR_D0            // move.w ccr,d0 legal on 68010+
        moveq   #10,d0

// CACR is present only in 68020+

        MOVEC_CACR_D1           // d1 = get cache control register
        move.l  d1,d2           // hold a copy for later
        ori.w   #0x0101,d1      // enable '020 instr. and '030 data caches
        MOVEC_D1_CACR           // set new cache controls from d1
        MOVEC_CACR_D1           //   & read it back again to check
        MOVEC_D2_CACR           // restore original CACR
        btst    #0,d1           // if '020 instr. cache was not enabled, this is a 68040+
        jeq     x040
        moveq   #20,d0          // assume 68020
        btst    #8,d1           // check if 68030 data cache was enabled
        jeq     exit            // a zero here means no data cache, i.e. 68020
        move.l  #0x0001001e,d0  // data cache enabled means 68030 - assume 68ec030
        PMOVE_FROM_TC(temp)     // try to access the TC register
        moveq   #30,d0          // no fault -> this is a 68030
        jra     exit

// 68040 or 68060 (or Apollo 68080)

x040:   moveq   #40,d0          // assume 68040
        MOVEC_PCR_D1            // attempt to access the PCR
        moveq   #60,d0          // no fault, assume 68060

#if CONF_WITH_APOLLO_68080
        ADDIWL_0_A0             // check for addiw.l instruction
// Apollo 68080
        moveq   #40,d0          // no fault, force 68040 detection
        move.w  #1,_is_apollo_68080
#endif

exit:
        move.l  a3,(0xf4).w     // restore stuff and exit
        move.l  a2,(0x2c).w
        move.l  a1,(0x10).w
        move.l  a0,sp
        nop                     // flush pipelines
        movem.l  (sp)+,d2/a2-a3
#else
        moveq   #0,d0           // say it's a 68000
#endif /* CONF_WITH_ADVANCED_CPU */
        rts

/*
 * _detect_fpu - FPU type detection, experimental (draco@atari.org).
 *
 * This can only detect the hardware FPU, any software emulation
 * will be ignored.
 *
 * NOTICE: the _FPU cookie value for 68060 is not defined by Atari!
 *         *** How does it look like on a Hades060? ***
 *
 * Return value is cookie value for _FPU slot or a zero if no FPU
 * is present:
 *
 * 0x00000000, no FPU
 * 0x00010000, memory mapped 68881 (SFP004)
 * 0x00020000, 68881 or 68882
 * 0x00040000, 68881 for sure
 * 0x00060000, 68882 for sure
 * 0x00080000, 68040 internal FPU
 * 0x00100000, 68060 internal FPU
 *
 * The detection algorithm goes as follows:
 *
 * - FRESTORE is executed. If a Line-F exception follows, then there's no FPU
 *   in coprocessor mode. If the CPU is >= 68020, no FPU is assumed.
 * - If FRESTORE doesn't take an exception, an FPU attached in coprocessor
 *   mode is present, that has been reset by FRESTORE. Then if 68040 or 68060
 *   CPU was detected previously, an appropriate FPU type is assumed.
 *   Otherwise the stackframe is checked for magic value indicating 68882 and
 *   if it is different, a 68881 is assumed.
 *
 * I am very interested if this will really work on everything =)
 * On a 68030/68882 tandem it does.
 */
_detect_fpu:
#if CONF_WITH_ADVANCED_CPU
        movem.l d2/a2,-(sp)
        move.l  sp,a0           // save the ssp
        moveq   #0,d0           // assume no FPU
        move.l  (0x2c).w,a1     // save the Line-F vector
        move.l  (0x08).w,a2
        move.l  #fexit,d1
        move.l  d1,(0x2c).w     // install temporary Line-F
        move.l  d1,(0x08).w
        nop                     // flush pipelines

        move.l  _mcpu,d1        // check if 68000 or 68010
        cmpi.w  #20,d1
#if CONF_WITH_SFP004
        jmi     sfp
#else
        jmi     fexit
#endif

        cmpi.w  #60,d1          // enable FPU on 68060 before the check
        jmi     no60

        MOVEC_PCR_D2            // get revision number etc
        swap    d2
        cmp.w   #0x0431,d2      // "broken" 68LC/EC060
        jeq     no60
        swap    d2
        bclr    #0x01,d2
        MOVEC_D2_PCR            // clear DFP (the disable-FPU bit)

no60:   clr.l   -(sp)           // push NULL frame
        clr.l   -(sp)           // extra longs for 68060
        clr.l   -(sp)
        FRESTORE_SP_PLUS        // frestore (sp)+   reset FPU into NULL state
        FNOP                    // fnop             force it into IDLE state
        FSAVE_MINUS_SP          // fsave -(sp)      save the IDLE frame

        moveq   #0x10,d0        // assume 68060 FPU (cookie 0x00100000)
        cmpi.w  #60,d1          // d1 is loaded above the FPU code
        jeq     fexit
        moveq   #0x08,d0        // if not 060, maybe 040 (cookie 0x00080000)
        cmpi.w  #40,d1
        jeq     fexit
        moveq   #0x06,d0        // if neither, maybe a 68882 (0x00060000)
        move.b  1(sp),d1        // get offset to last long of IDLE frame
        cmpi.b  #0x38,d1        // is it a 68882?
        jeq     fexit           // yes, branch
        moveq   #0x04,d0        // must be 68881

#if CONF_WITH_SFP004
        jra     fexit
sfp:    tst.w   0xfffffa40.w    // CIR
        moveq   #0x01,d0        // memory mapped FPU
#endif

fexit:  move.l  a1,(0x2c).w     // restore Line-F
        move.l  a2,(0x08).w
        move.l  a0,sp
        nop                     // flush pipelines
        swap    d0
        movem.l (sp)+,d2/a2

#else
        // handle only 68000 (with possible SFP004)
        moveq   #0,d0           // assume no FPU

#if CONF_WITH_SFP004
        move.l  sp,a0           // save the ssp
        move.l  (0x08).w,a1     // save the bus error vector
        move.l  #nosfp,d1
        move.l  d1,(0x08).w     // install temporary bus error vector
        nop                     // flush pipelines
        tst.w   0xfffffa40.w    // CIR
        moveq   #0x01,d0        // memory mapped FPU
nosfp:  move.l  a1,(0x08).w     // restore bus error
        move.l  a0,sp
        nop                     // flush pipelines
        swap    d0
#endif

#endif /* CONF_WITH_ADVANCED_CPU */
        rts

#endif /* __mcoldfire__ */


/*
 * void processor_init(void) - sets mcpu and fputype.
 */
_processor_init:
#ifndef __mcoldfire__           // On ColdFire, the caches are enabled by the pre-OS
        jbsr    _detect_cpu
        swap    d0
        move.w  d0,_mcpu_subtype    // save processor subtype
        clr.w   d0
        swap    d0
        move.l  d0,_mcpu            // & type
#if CONF_WITH_ADVANCED_CPU
        jeq     m68000

        move.w  #1,_longframe.w // this is a 68010 or later
m68000:
        jbsr    _detect_fpu
        move.l  d0,_fputype

        cmpi.b  #20,_mcpu+3
        jne     pi_chk030

// We're running on a 68020
        move.l  #0x00000009,d0  // clear & enable cache
        jra     init_cacr

pi_chk030:
        cmpi.b  #30,_mcpu+3
        jne     pi_chk040

// We're running on a 68030 or a 68ec030

#if CONF_WITH_68030_PMMU

// User would like a PMMU tree, so see if it's possible

        tst.w   _mcpu_subtype   // check for full 68030
        jne     m68ec030

// On a full 68030, if a PMMU tree is wanted, we must set it up before
// enabling the data cache.  The PMMU tree will ensure that caching is
// inhibited for access to i/o addresses.

        jsr     _setup_68030_pmmu
        PMOVE_TO_CRP(root_pointer_descriptor)   // tell system where tree is
        PMOVE_TO_TC(init_tc)        // enable PMMU
        PMOVE_TO_TTR0(init_ttr0)    // override PMMU tree for accesses to
        PMOVE_TO_TTR1(init_ttr1)    //   0x01000000-0xfeffffff (see below for details)
        move.l  #0x00003919,d0      // we set WA, DBE, CD/ED, IBE and CI/EI
        jra     init_cacr

m68ec030:

#endif

// If user doesn't want a PMMU tree, or we're on a 68ec030, we enable
// just the instruction cache, since we can't set up a proper PMMU
// tree to avoid caching the I/O address area

        move.l  #0x00000019,d0  // set IBE and CI/EI only
        jra     init_cacr

pi_chk040:
        move.l  #0x80008000,d1  // set 040 CACR value: DE and IE
        cmpi.b  #40,_mcpu+3
        jeq     m68040
        move.l  #0x80c08000,d1  // set 060 CACR value: EDC, EBC/CABC and EIC
        cmpi.b  #60,_mcpu+3
        jeq     m68060
        jra     pi_done

m68040:
// the optional 68040 MMU initialization used to be done here.  it is
// now done later because we need to know _ramtop first, and that is
// set later by ttram_detect()

m68060:
//
// On a 68040 or a 68060:
//   disable the PMMU (it might be enabled if we're running as a PRG)
//   set up the TTRs
//   clear the caches
//   enable the caches (on the 68060, we also enable the branch cache)
//

        moveq   #0,d0
        MOVEC_D0_TC             // disable translation

        // Instruction Transparent Translation Registers (ITTRs):
        //  ITTR 0: all addresses, cachable/write-through, user & supervisor
        //  ITTR 1: disabled
        move.l  #0x00ffe000,d0
        MOVEC_D0_ITT0           // set up ITTR 0
        moveq   #0,d0
        MOVEC_D0_ITT1           // and ITTR1

        // Data Transparent Translation Registers (DTTRs):
        //  DTTR 0: all addresses, cache-inhibited/precise, supervisor mode
        //  DTTR 1: all addresses, cache-enabled/write-through, user mode
        move.l  #0x00ffa040,d0
        MOVEC_D0_DTT0           // set up DTTR0
        move.l  #0x00ff8000,d0
        MOVEC_D0_DTT1           // and DTTR1

        nop
        CINVA_BC                // invalidate the caches
        nop
        move.l  d1,d0           // recover the enable value

init_cacr:
#if CONF_WITH_CACHE_CONTROL
        move.l  d0,cacr_enable  // save value to use when enabling caches
#endif
        MOVEC_D0_CACR

pi_done:
#endif /* CONF_WITH_ADVANCED_CPU */

#endif /* not __mcoldfire__ */
        rts

/*
 * void instruction_cache_kludge(void *start, long size)
 *
 * TOS compatibility: invalidate the instruction cache
 *
 * this provides backward compatibility in case some foolish person
 * reads code from an I/O device and branches to it directly; this
 * would have been legal on STs and STes.
 *
 * we don't do that on ColdFire, because ColdFire executables are brand new
 * and supposed to be aware of cache issues.
 */
_instruction_cache_kludge:
#ifdef __mcoldfire__
        rts
#endif
//
// for 680x0, we fall through invalidate_instruction_cache
//

/*
 * void invalidate_instruction_cache(void *start, long size)
 * First, the data cache is flushed to push changes into the RAM.
 * Then the instruction cache is invalidated for the specified zone.
 *
 * We're cautious here and invalidate all the cache. Since not all systems
 * map all memory 1:1, being more precise would require either knowing the
 * physical address of line(s) to be cleaned/invalidated, or performing address
 * translation on the range.
 * This quickly becomes more expensive than just cleaning the caches and paying
 * the cost to repopulate them later.
 */

_invalidate_instruction_cache:
#ifdef __mcoldfire__
        lea     cpushl_bc,a1    // flush/invalidate both caches
        jra     cpushl_loop
#else
#if CONF_WITH_ADVANCED_CPU
        cmpi.b  #20,_mcpu+3
        jeq     ii_020or030
        cmpi.b  #30,_mcpu+3
        jne     ii_not30

ii_020or030:
        MOVEC_CACR_D0                   // get current cacr
        ori.b   #0x08,d0                // set the CI bit
        MOVEC_D0_CACR                   // clear the whole i-cache
        jra     ii_done

ii_not30:
        cmp.b   #40,_mcpu+3
        jeq     ii_inval
        cmpi.b  #60,_mcpu+3
        jne     ii_done

ii_inval:
        // If the 040/060 MMU is enabled, or the DTTs have been configured
        // for copyback mode, there may be dirty lines in the data cache
        // containing instructions, so we need to flush the data cache as well
        // as invalidating the instruction cache.
        nop
        CPUSHA_BC
        nop
ii_done:
#endif /* CONF_WITH_ADVANCED_CPU */
        rts
#endif


/*
 * void flush_data_cache(void *start, long size)
 *
 * flush data cache before writing data with DMA
 *
 * the actions required depend on the mode of data cache:
 *   write-through:
 *     no action is necessary
 *   copyback:
 *     we must push the data cache (the backing memory may be stale)
 */
_flush_data_cache:
#ifdef __mcoldfire__
        lea     cpushl_dc,a1    // flush/invalidate data cache
        jra     cpushl_loop
#else
#if CONF_WITH_ADVANCED_CPU

        // 68020 has no data cache, 68030 data cache is always write-through,
        // in either case no action is necessary.
        cmpi.b  #40,_mcpu+3
        jmi     fd_no_dirty

        // 68040/68060 data caches are either write-through or copyback
        // depending on how the DTTs / MMU are set up, so we need to push them
        // here.
        nop
        CPUSHA_DC
        nop
fd_no_dirty:
#endif
        rts
#endif


/*
 * void invalidate_data_cache(void *start, long size)
 *
 * invalidate data cache after data has been read with DMA
 *
 * for both modes of data cache (write_through and copyback),
 * the cache needs to be invalidated
 */
_invalidate_data_cache:
#ifdef __mcoldfire__
        lea     cpushl_dc,a1    // flush/invalidate data cache
        // fall through cpushl_loop
#else
#if CONF_WITH_ADVANCED_CPU
        cmpi.b  #30,_mcpu+3
        jne     ic_not30
        MOVEC_CACR_D0                   // get current cacr
        ori.w   #0x0800,d0              // set CD bit
        MOVEC_D0_CACR                   // clear data cache
        jra     ic_done
ic_not30:
        cmpi.b  #40,_mcpu+3
        jeq     ic_inval
        cmpi.b  #60,_mcpu+3
        jne     ic_done

ic_inval:
        // We can't simply invalidate the data cache, as there may
        // be dirty lines resident, so we need to push those out.
        // Note: this requires that the 060's CACR.DPI bit is not set
        //       in order for lines to be invalidated.
        nop
        CPUSHA_DC
        nop
ic_done:
#endif /* CONF_WITH_ADVANCED_CPU */
        rts
#endif

#ifdef __mcoldfire__
cpushl_loop:
        // This helper routine is a loop around cpushl ?c,(a0)
        // a1 must point to the actual instruction called in the loop

        // This code comes from the MCF547x Reference Manual
        // Section 7.11 Cache Management
        //
        // The ColdFire has no cinva instruction.
        // Instead, cpushl writes the modified cache data to the RAM
        // then invalidates the caches (data + instruction) except if
        // the DDPI and IDPI bits have been set in the CACR.
        //
        // The ColdFire V4e core has a 32 KB instruction cache
        // and a 32 KB data cache. Both caches have the same structure.
        // The data is stored in "Line" elements of 16 bytes.
        // The Lines are stored in a 2D array of 4 Ways * 512 Sets.
        //
        // The following changes have been made to the original code:
        // - call jsr (a1) instead of "cpushl dc"
        // - flush the 512 Sets (original code forgot the last one)

        nop                     // synchronize/flush store buffer
        moveq.l #0,d0           // initialize way counter
        moveq.l #0,d1           // initialize set counter
        move.l  d0,a0           // initialize cpushl pointer

setloop:
        jsr     (a1)            // call appropriate cpushl instruction
        lea     0x0010(a0),a0   // increment set index by 1
        addq.l  #1,d1           // increment set counter
        cmpi.l  #512,d1         // are sets for this way done?
        jne     setloop

        moveq.l #0,d1           // set counter to zero again
        addq.l  #1,d0           // increment to next way
        move.l  d0,a0           // set = 0, way = d0
        cmpi.l  #4,d0           // flushed all the ways?
        jne     setloop

        rts

// Helper routines for cpushl_loop

cpushl_ic:
        cpushl  ic,(a0)         // instruction cache
        rts

cpushl_dc:
        cpushl  dc,(a0)         // data cache
        rts

cpushl_bc:
        cpushl  bc,(a0)         // both caches
        rts
#endif


#if CONF_WITH_CACHE_CONTROL
/*
 * C A C H E   C O N T R O L   F O R   E M U D E S K
 *
 * WORD cache_exists(void);
 *  returns 1 if processor has cache, 0 otherwise
 *
 * void set_cache(WORD enable);
 *  sets cache state: if 'enable' is zero, disables, otherwise enables
 */
_cache_exists:
        moveq.l #0,d0               // assume 68000, 68010
        cmpi.b  #20,_mcpu+3
        jmi     ce_exit
        moveq.l #1,d0
ce_exit:
        rts

_set_cache:
        jbsr    _cache_exists
        tst.w   d0
        jeq     sc_exit             // no cache, do nothing
        moveq.l #0,d0               // assume disable
        tst.w   4(sp)
        jne     sc_enable           // no, enable
        cmpi.b  #40,_mcpu+3         // possible dirty data in cache?
        jmi     sc_cacr             // no, go do it
        nop
        CPUSHA_DC                   // 040/060: yes, clean the data cache first
        nop
        jra     sc_cacr             // now go do it
sc_enable:
        move.l  cacr_enable,d0      // enabling: set processor-appropriate value
        cmpi.b  #40,_mcpu+3         // 040/060 ?
        jmi     sc_cacr             // no, just go set cacr
        nop
        CINVA_BC                    // 040/060: invalidate the caches first
        nop
sc_cacr:
        MOVEC_D0_CACR
sc_exit:
        rts
#endif

        SECTION_RODATA

#if CONF_WITH_68030_PMMU
        .even   // Mandatory in ELF section .rodata
root_pointer_descriptor:
        .dc.l   0x80000002,_pmmutree // lower limit 0, short format, table at '_pmmutree'
init_tc:
        .dc.l   0x80F04445          // enable, 32K pages, table indexes=4/4/4/5
init_ttr0:                          // transparent translation is enabled for all FCs
        .dc.l   0x017E8107          //   for 0x01000000-0x7fffffff, caching allowed
init_ttr1:                          // transparent translation is enabled for all FCs
        .dc.l   0x807E8507          //   for 0x80000000-0xfeffffff, *caching inhibited*
#endif

// ==== Variables ============================================================

        .bss
        .even

#ifndef __mcoldfire__
_mcpu:          .ds.l   1
_mcpu_subtype:  .ds.w   1
_fputype:       .ds.l   1
temp:           .ds.l   1
#endif

#if CONF_WITH_APOLLO_68080
_is_apollo_68080:   .ds.w   1
#endif

#if CONF_WITH_CACHE_CONTROL
cacr_enable:    .ds.l   1
#endif
