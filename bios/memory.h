/*
 * memory.h - Memory functions
 *
 * Copyright (C) 2016-2022 The EmuTOS development team
 *
 * Authors:
 *  VRI   Vincent Rivière
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#ifndef MEMORY_H
#define MEMORY_H

/* Bit definitions for meminit_flags */

#define MEMINIT_BIT_FIRST_BOOT 0
/* If this bit is set, this means it is the first time that this EmuTOS
 * instance is run. This is always true for a cold boot from ROM. This is
 * always false for a reset. If EmuTOS lives in RAM, it is only true the first
 * time it is run. */

#if CONF_WITH_FALCON_MMU
# define MEMINIT_BIT_FALCON_MMU 1 /* This machine has Falcon MMU */
#endif

#if CONF_WITH_TT_MMU
# define MEMINIT_BIT_TT_MMU 2 /* This machine has TT MMU */
#endif

#if CONF_WITH_ST_MMU
# define ST_MMU_CONFIGURATION 0xffff8001L // byte
#endif


#ifndef ASM_SOURCE

void meminit(PFVOID return_address);
void ttram_detect(void);

#if CONF_WITH_ALT_RAM

void altram_init(void);

#endif /* CONF_WITH_ALT_RAM */

#if CONF_WITH_MEMORY_TEST
BOOL memory_test(void);
BOOL memtest_verify(ULONG *start, ULONG value, LONG length);    /* in memory.S */
BOOL memtest_rotate_verify(ULONG *start, LONG length);          /* in memory.S */
#endif

/* These flags will be set up early by meminit() */
extern UBYTE meminit_flags;
#define MEMINIT_FIRST_BOOT (1 << MEMINIT_BIT_FIRST_BOOT)

#if CONF_WITH_FALCON_MMU
# define MEMINIT_FALCON_MMU (1 << MEMINIT_BIT_FALCON_MMU)
#endif

#if CONF_WITH_TT_MMU
# define MEMINIT_TT_MMU (1 << MEMINIT_BIT_TT_MMU)
#endif

#endif /* ASM_SOURCE */

#endif /* MEMORY_H */
