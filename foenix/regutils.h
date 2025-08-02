/* Public domain */

#ifndef __REGUTILS_H__
#define __REGUTILS_H__

#ifndef R8
    #define R8(x) *((volatile int8_t * const)(x))
#endif
#ifndef R16
    #define R16(x) *((volatile uint16_t * const)(x))
#endif
#ifndef R32
    #define R32(x) *((volatile uint32_t * const)(x))
#endif

/* It's critical to read and write registers with the correct size. This size depends on the
 * data bus width of the CPU so unfortunately it's not the same for the 68000 (16bits) and 68040 (32bits).
 * The following macros help abstract this. */
#ifdef MACHINE_A2560U
    #define GAVIN_R R16
#else
    #define GAVIN_R R32
#endif

/* GCC will happily optimize out an empty wait loop. Put this in the loop to work around it.
 * e.g. for(int i=0;i<200000;i++) DONT_OPTIMIZE OUT;*/
#define DONT_OPTIMIZE_OUT __asm("")

#endif /* __REGUTILS_H__ */