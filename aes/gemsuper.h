/*
 * gemsuper.h - header for EmuTOS AES function call handler
 *
 * Copyright (C) 2002-2022 The EmuTOS development team
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#ifndef GEMSUPER_H
#define GEMSUPER_H

typedef struct aespb {
    WORD *control;
    WORD *global;
    WORD *intin;
    WORD *intout;
    LONG *addrin;
    LONG *addrout;
} AESPB;

#if CONF_AES_INPROCESS
WORD aes_inprocess_call(AESPB *pb);
/* Track the in-process desk while it is inside aes_inprocess_call().
 * unsync() must not dsptch() when *that* PD is the releaser — control
 * would never return to deskmain.  If another PD (e.g. ctlmgr) releases
 * while the desk is blocked in mwait inside aes_inprocess_call, dsptch
 * is still required so the desk can resume. */
extern WORD aes_inprocess_depth;
struct aespd;
extern struct aespd *aes_inprocess_pd;
#endif

#endif
