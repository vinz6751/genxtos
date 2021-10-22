/*
 * os_xheader.S - EmuTOS extended OS header
 *
 * Copyright (C) 2001-2021 The EmuTOS development team
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 *
 * This file is included in startup.S
 */

 // ==== Definitions ==========================================================

        .globl  _osxhbootdelay

/*
 * Feature flags in extended OS header (see below)
 *
 *  Bit #       Usage
 *    0         EmuTOS will pass the address of the lineA variables via NatFeats
 *   1-31       (currently reserved)
 */
#if DETECT_NATIVE_FEATURES
# define OSXH_LINEA     (1<<0)
#else
# define OSXH_LINEA     0
#endif
#define OSXH_FEATURES   (OSXH_LINEA)

/*
 *  Machine flags in extended OS header
 *
 *  Bit #       Supported machines
 *    0         ST / MegaST
 *    1         STe / MegaSTe
 *    2         TT
 *    3         Falcon
 *    4         FireBee
 *    5         M548X evaluation boards
 *    6         Amiga
 *    7         (currently reserved)
 *    8         ARAnyM
 *    9         Foenix
 *   10-15       (currently reserved)
 */
/* Set OS extended header machine type */
#if defined(MACHINE_ARANYM)
# define OSXH_ARANYM    (1<<8)
#elif defined(MACHINE_AMIGA)
# define OSXH_AMIGA     (1<<6)
#elif defined(MACHINE_M548X)
# define OSXH_M548X     (1<<5)
#elif defined(MACHINE_FIREBEE)
# define OSXH_FIREBEE   (1<<4)
#elif defined(OSXH_C256FOENIXGENX)
# define OSXH_C256FOENIXGENX   (1<<9)
#else
# ifndef __mcoldfire__
#  define OSXH_ST       (1<<0)
# endif
# if CONF_WITH_STE_SHIFTER
#  define OSXH_STE      (1<<1)
# endif
# if CONF_WITH_TT_SHIFTER
#  define OSXH_TT       (1<<2)
# endif
# if CONF_WITH_VIDEL
#  define OSXH_FALCON   (1<<3)
# endif
# if CONF_WITH_ARANYM
#  define OSXH_ARANYM   (1<<8)
# endif
#endif
/* Default values for what's not set */
# if !defined(OSXH_ST)
#  define OSXH_ST       0
# endif
# if !defined(OSXH_STE)
#  define OSXH_STE      0
# endif
# if !defined(OSXH_TT)
# define OSXH_TT        0
# endif
# if !defined(OSXH_FALCON)
# define OSXH_FALCON    0
# endif
# if !defined(OSXH_FIREBEE)
# define OSXH_FIREBEE   0
# endif
# if !defined(OSXH_M548X)
# define OSXH_M548X     0
# endif
# if !defined(OSXH_AMIGA)
# define OSXH_AMIGA     0
# endif
# if !defined(OSXH_ARANYM)
# define OSXH_ARANYM    0
# endif
# if !defined(OSXH_C256FOENIXGENX)
# define OSXH_C256FOENIXGENX 0
# endif

#define OSXH_MACHINES   (OSXH_ST+OSXH_STE+OSXH_TT+OSXH_FALCON+OSXH_FIREBEE+OSXH_M548X+OSXH_ARANYM+OSXH_AMIGA+OSXH_C256FOENIXGENX)

/*
 *  Processor flags in extended OS header
 *
 *  Bit #       Supported processors
 *    0         68000
 *    1         68010
 *    2         68020
 *    3         68030
 *    4         68040
 *    5         68060
 *    6         68080
 *    7         (currently reserved)
 *    8         ColdFire V4e
 *   9-15       (currently reserved)
 */
#if defined(MACHINE_ARANYM)
# define OSXH_68000     0
# define OSXH_68010     0
# define OSXH_68020     0
# define OSXH_68030     0
# define OSXH_68040     (1<<4)
# define OSXH_68060     0
# define OSXH_68080     0
# define OSXH_COLDFIRE  0
#elif defined(MACHINE_M548X)
# define OSXH_68000     0
# define OSXH_68010     0
# define OSXH_68020     0
# define OSXH_68030     0
# define OSXH_68040     0
# define OSXH_68060     0
# define OSXH_68080     0
# define OSXH_COLDFIRE  (1<<8)
#elif defined(MACHINE_FIREBEE)
# define OSXH_68000     0
# define OSXH_68010     0
# define OSXH_68020     0
# define OSXH_68030     0
# define OSXH_68040     0
# define OSXH_68060     0
# define OSXH_68080     0
# define OSXH_COLDFIRE  (1<<8)
#else
# define OSXH_COLDFIRE  0
# ifndef __mcoldfire__
#  define OSXH_68000    (1<<0)
# else
#  define OSXH_68000    0
# endif
# if CONF_WITH_ADVANCED_CPU
#  define OSXH_68010    (1<<1)
#  define OSXH_68020    (1<<2)
#  define OSXH_68030    (1<<3)
#  define OSXH_68040    (1<<4)
#  define OSXH_68060    (1<<5)
# else
#  define OSXH_68010    0
#  define OSXH_68020    0
#  define OSXH_68030    0
#  define OSXH_68040    0
#  define OSXH_68060    0
# endif
# if CONF_WITH_APOLLO_68080
#  define OSXH_68080    (1<<6)
# else
#  define OSXH_68080    0
# endif
#endif
#define OSXH_PROCESSORS (OSXH_68000+OSXH_68010+OSXH_68020+OSXH_68030+OSXH_68040+OSXH_68060+OSXH_68080+OSXH_COLDFIRE)

/*
 * Default cold-boot delay. Can be modified by users to allow
 * harddisks to become ready. 0 means no delay.
 */
#define OSXH_BOOTDELAY  (0)

/*
 * Extended OS header
 *
 * This is an EmuTOS extension, present from EmuTOS version 1.0 on.  You
 * may check for its presence by checking the first 4 bytes which contain
 * the magic number 'OSXH'.
 *
 * To avoid problems with programs that branch directly to _main, rather
 * than using the branch instruction at the beginning of the TOS os_header,
 * there is a 4-byte branch instruction at _main which branches past the
 * extended header.  This means, for example, that the extended OS header
 * begins at 0xfc0034 in the 192K ROMS, and at 0xe00034 in the 256K and
 * 512K ROMs.
 */
osxh:
    .ascii  "OSXH"                      // magic number
    .dc.l   osxhend-osxh                // length
    .dc.b   MAJOR_VERSION,MINOR_VERSION,FIX_VERSION,UNOFFICIAL  // binary version
    .dc.l   _version-_os_header         // offset to version string
    .dc.l   OSXH_FEATURES               // feature flags
    .dc.w   OSXH_MACHINES               // machine flags
    .dc.w   OSXH_PROCESSORS             // processor flags
_osxhbootdelay:
    .dc.b   OSXH_BOOTDELAY              // cold-boot delay
osxhend:
    .even                               // for alignment
