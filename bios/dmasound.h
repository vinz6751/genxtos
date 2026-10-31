/*
 * dmasound.h - STe/TT/Falcon DMA sound routines
 *
 * Copyright (C) 2011-2016 The EmuTOS development team
 *
 * Authors:
 *  VRI   Vincent Rivière
 *  THH   Thomas Huth
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#ifndef DMASOUND_H
#define DMASOUND_H

#if CONF_WITH_DMASOUND

#define DMASOUND_IRQ  0xffff8900L
#define DMASOUND_CTRL 0xffff8901L

#define SNDNOTLOCK  -128        /* error codes from Unlocksnd(), Locksnd() */
#define SNDLOCKED   -129

void detect_dmasound(void);
void dmasound_init(void);

/* XBIOS DMA sound functions */
LONG ARGS_ON_STACK locksnd(void);
LONG ARGS_ON_STACK unlocksnd(void);
LONG ARGS_ON_STACK soundcmd(WORD mode, WORD data);
LONG ARGS_ON_STACK setbuffer(UWORD mode, ULONG startaddr, ULONG endaddr);
LONG ARGS_ON_STACK setsndmode(UWORD mode);
LONG ARGS_ON_STACK settracks(UWORD playtracks, UWORD rectracks);
LONG ARGS_ON_STACK setmontracks(UWORD montrack);
LONG ARGS_ON_STACK setinterrupt(UWORD mode, WORD cause);
LONG ARGS_ON_STACK buffoper(WORD mode);
LONG ARGS_ON_STACK dsptristate(WORD dspxmit, WORD dsprec);
LONG ARGS_ON_STACK gpio(UWORD mode, UWORD data);
LONG ARGS_ON_STACK devconnect(WORD source, WORD dest, WORD clk, WORD prescale, WORD protocol);
LONG ARGS_ON_STACK sndstatus(WORD reset);
LONG ARGS_ON_STACK buffptr(LONG sptr);

#endif /* CONF_WITH_DMASOUND */

#endif /* DMASOUND_H */
