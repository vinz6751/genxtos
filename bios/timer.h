/*
 * timer.c - OS-level functions related to timers
 *
 * Copyright (C) 2001 Martin Doering
 * Copyright (C) 2001-2024 The EmuTOS development team
 *
 * Authors:
 *  LVL   Laurent Vogel
 *  MAD   Martin Doering
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */


#ifndef TIMER_H
#define TIMER_H

#include "portab.h"


/* Related TOS variables, we're not declaring them because they're TOS variables
 * so they're already declared in tosvars.h */
//extern WORD timer_ms;
//extern LONG hz_200;
#include "tosvars.h"

/* Non-Atari hardware vectors */
#if !CONF_WITH_MFP
extern void (*vector_5ms)(void);              /* 200 Hz system timer */
#endif

void init_system_timer(void);
void int_timerc(void);

/* Return FALSE if the predicate didn't return true within the given delay (in 200Hz ticks) */
BOOL timer_test_with_timeout(BOOL (*predicate)(void), LONG delay);

/* Start processing the routines to be executed every 5ms */
void timer_start_20ms_routine(void);

/* XBIOS function */
void xbtimer(WORD timer, WORD control, WORD data, LONG vector);

#endif