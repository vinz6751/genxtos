/*
 * natfeat.h - NatFeat header file
 *
 * Copyright (C) 2001-2020 The EmuTOS development team
 *
 * Authors:
 *  PES   Petr Stehlik
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#ifndef _NATFEAT_H
#define _NATFEAT_H

#include "portab.h"

/*
 * NatFeat nfID/nfCall stubs expect stack arguments (Aranym/Hatari convention).
 * Under FASTCALL_BUILD, function pointers must be declared cdecl or lookups fail.
 */
#if defined(FASTCALL_BUILD)
typedef long (ARGS_ON_STACK *NFIdFunc)(const char *name);
typedef long (ARGS_ON_STACK *NFCallFunc)(long id, ...);
#else
typedef long (*NFIdFunc)(const char *name);
typedef long (*NFCallFunc)(long id, ...);
#endif

typedef struct
{
        long magic;
        NFIdFunc nfID;
        NFCallFunc nfCall;
} NatFeatCookie;

extern NatFeatCookie natfeat_cookie;

#define NFID    natfeat_cookie.nfID
#define NFCall  natfeat_cookie.nfCall

void natfeat_init(void);
BOOL has_natfeats(void);

long nfGetFullName(char *buffer, long size);

BOOL is_nfStdErr(void);
long nfStdErr(const char *text);

BOOL has_nf_debugger(void);
void nf_debugger(void);

unsigned long nf_cycles(void);

BOOL has_nf_exit(void);
void nf_exit(long exitval);

long get_xhdi_nfid(void);

/* terminate the execution of the emulator if possible, else no-op */
void nf_shutdown(void);
BOOL has_nf_shutdown(void);

BOOL has_nf_bootstrap(void);

/* load a new OS kernel into the memory to 'addr' ('size' bytes available) */
long nf_bootstrap(UBYTE *addr, long size);
/* return the OS kernel arguments to be passed to 'addr' ('size' bytes available) */
long nf_getbootstrap_args(char *addr, long size);
UWORD nf_getbootdrive(void);

/* check if the host is emulating the MMU */
BOOL mmu_is_emulated(void);

/* propagate address of linea variables to emulators */
void nf_setlinea(void);

#endif /* _NATFEAT_H */
