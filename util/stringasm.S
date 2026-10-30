/*
 * stringasm.S - simple implementation of <string.h>-like routines
 *
 * Copyright (C) 2002-2020 The EmuTOS development team
 *
 * Authors:
 *  LVL   Laurent Vogel
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#include "asmdefs.h"

        .globl  _strlencpy
        .globl  _strchr

/*
 * WORD strlencpy(char *RESTRICT dest, const char *RESTRICT src)
 * as strcpy but returns the length of the string instead.
 */

_strlencpy:
#ifdef FASTCALL_BUILD
        exg     a0,a1
#else
        movea.l 8(sp),a0
        movea.l 4(sp),a1
#endif
        moveq   #0,d0
lstcpy_loop:
#ifdef __mcoldfire__
        addq.l  #1,d0
#else
        addq.w  #1,d0
#endif
        move.b  (a0)+,(a1)+
        jne     lstcpy_loop
#ifdef __mcoldfire__
        subq.l  #1,d0
#else
        subq.w  #1,d0
#endif
        rts

/*
 * char *strchr(const char *s, int c)
 *
 * returns a pointer to the first occurrence of char c in string s, or NULL.
 */

_strchr:
#ifdef FASTCALL_BUILD
        move.w  d0,d1
#else
        move.l  4(sp),a0
        move.w  8(sp),d1
#endif
strchr_loop:
        move.b  (a0)+,d0
        jeq     strchr_null
        cmp.b   d1,d0
        jne     strchr_loop
        subq.l  #1,a0
#ifdef FASTCALL_BUILD
        rts                     // pointer return already in a0
#else
        move.l  a0,d0
        rts
#endif
strchr_null:
#ifdef FASTCALL_BUILD
        suba.l  a0,a0           // return NULL in a0
        rts
#else
        moveq.l #0,d0
        rts
#endif
