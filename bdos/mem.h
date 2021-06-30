/*
 * mem.h - header file for memory and process management routines
 *
 * Copyright (C) 2001 Lineo, Inc. and
 *               2002-2019 The EmuTOS development team
 *
 * Authors:
 *  LVL Laurent Vogel
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#ifndef MEM_H
#define MEM_H

#include "bdosdefs.h"


/*
 *  externals
 */

extern  MEMORY_PARTITION_BLOCK     pmd;    /* the mem pool for the main user ST ram */
#if CONF_WITH_ALT_RAM
extern  MEMORY_PARTITION_BLOCK     pmdalt;  /* the memory pool for the alternative ram (TT-RAM or other) */
#endif

/*
 * Alignment of malloc'ed memory.
 * This value must be set to 2^n-1 to align on multiples of 2^n.
 */
extern  ULONG   malloc_align_stram;
#define MALLOC_ALIGN_ALTRAM     3

/*
 * in osmem.c
 */

/*  xmgetblk - get a block of memory from the o/s pool. */
void *xmgetblk(WORD memtype);

/*  MGET - wrapper around xmgetblk */
#define MGET(x)         ((x *)xmgetblk(MEMTYPE_ ## x))
#define MEMTYPE_MDBLOCK 0   /* the 4 types of valid request, all needing 64 bytes */
#define MEMTYPE_DMD     1
#define MEMTYPE_DND     2
#define MEMTYPE_OFD     3

/*  xmfreblk - free up memory allocated through mgetblk */
void xmfreblk(void *m);

MEMORY_DESCRIPTOR *xmgetmd(void);          /* xmgetmd - get an MEMORY_DESCRIPTOR */
void xmfremd(MEMORY_DESCRIPTOR *md);       /* xmfremd - free an MEMORY_DESCRIPTOR */

/* init os memory */
void osmem_init(void);

/*
 * in umem.c
 */

/* allocate memory */
void *xmalloc(long amount);
/* mfree */
long xmfree(void *addr);
/* mshrink */
long xsetblk(int n, void *blk, long len);
/* mxalloc */
void *xmxalloc(long amount, int mode);
/* srealloc */
void *srealloc(long amount);

/* init user memory */
void umem_init(void);

/* set memory ownership */
void set_owner(void *addr, PD *p);


/*
 * in iumem.c
 */

/* find first fit for requested memory in ospool */
MEMORY_DESCRIPTOR *ffit(long amount, MEMORY_PARTITION_BLOCK *mp);
/* Free up a memory descriptor */
void freeit(MEMORY_DESCRIPTOR *m, MEMORY_PARTITION_BLOCK *mp);
/* shrink a memory descriptor */
WORD shrinkit(MEMORY_DESCRIPTOR *m, MEMORY_PARTITION_BLOCK *mp, LONG newlen);


#endif /* MEM_H */
