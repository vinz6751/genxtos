/*
 * kprint.h - BIOS console routines & debug macros
 *
 * Copyright (C) 2001-2020 The EmuTOS development team
 *
 * Authors:
 *  MAD     Martin Doering
 *  LVL     Laurent Vogel
 *  VRI     Vincent Rivière
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#ifndef KPRINT_H
#define KPRINT_H

#include <stdarg.h>
#include "portab.h"
#include "config.h"

#if DETECT_NATIVE_FEATURES
#include "natfeat.h"
#endif

/* console output */
int VARARGS cprintf(const char *RESTRICT fmt, ...) PRINTF_STYLE;

/* native debugging output */
int VARARGS kprintf(const char *RESTRICT fmt, ...) PRINTF_STYLE;

/* same thing, but taking a val_list */
int vkcprintf(const char *fmt, va_list ap);

/* output done both through kprintf and cprintf */
int VARARGS kcprintf(const char *RESTRICT fmt, ...) PRINTF_STYLE;

/* KINFO(()) outputs to the debugger, if kprintf() is available */
#if HAS_KPRINTF
#define KINFO(args) kprintf args
#else
#define KINFO(args) NULL_FUNCTION()
#endif

/* KDEBUG(()) may call KINFO(()) when locally enabled */
#ifdef ENABLE_KDEBUG
#define KDEBUG(args) KINFO(args)
#else
#define KDEBUG(args) NULL_FUNCTION()
#endif

/* A handy macro used when debugging */
#ifdef __GNUC__
#define HERE kprintf("HERE %s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__);
#else
#define HERE kprintf("HERE %s:%d\n", __FILE__, __LINE__);
#endif

/*
 * NatFeat-assisted debugging (Hatari/ARAnyM with --natfeats on).
 * NF_TRACE  -> kprintf routed to NF_STDERR when available
 * NF_BREAK  -> invoke host debugger (NF_DEBUGGER)
 * NF_ASSERT -> trace + debugger on failure
 * NFDBG     -> NF_TRACE, compiled only with NATFEAT_DEBUG=1
 */
#if DETECT_NATIVE_FEATURES
#define NF_TRACE(args) \
    do { if (is_nfStdErr()) kprintf args; } while(0)
#ifdef NATFEAT_DEBUG
#define NFDBG(args) NF_TRACE(args)
#else
#define NFDBG(args) NULL_FUNCTION()
#endif
#define NF_BREAK() \
    do { if (has_nf_debugger()) nf_debugger(); } while(0)
#define NF_ASSERT(expr) \
    do { \
        if (!(expr)) { \
            kprintf("NF_ASSERT failed: %s:%d: %s\n", __FILE__, __LINE__, #expr); \
            if (has_nf_debugger()) nf_debugger(); \
        } \
    } while(0)
#ifdef __GNUC__
#define NF_HERE() NF_TRACE(("NF_HERE %s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__))
#else
#define NF_HERE() NF_TRACE(("NF_HERE %s:%d\n", __FILE__, __LINE__))
#endif
#else
#define NF_TRACE(args) NULL_FUNCTION()
#define NFDBG(args) NULL_FUNCTION()
#define NF_BREAK() NULL_FUNCTION()
#define NF_ASSERT(expr) NULL_FUNCTION()
#define NF_HERE() NULL_FUNCTION()
#endif

/*
 * Numbered desktop boot milestones (grep "DESK-MS" in hatari-nftrace.log).
 * Compiled only with NATFEAT_DEBUG=1; build with NATFEAT_DEBUG=1 make 256.
 */
#ifdef NATFEAT_DEBUG
#define DESK_BOOT_MS(n, msg) NF_TRACE(("DESK-MS %02u: %s\n", (unsigned)(n), (msg)))
#define DESK_BOOT_MSF(n, fmt, ...) NF_TRACE(("DESK-MS %02u: " fmt "\n", (unsigned)(n), __VA_ARGS__))
#else
#define DESK_BOOT_MS(n, msg) NULL_FUNCTION()
#define DESK_BOOT_MSF(n, msg, ...) NULL_FUNCTION()
#endif

#endif /* KPRINT_H */
