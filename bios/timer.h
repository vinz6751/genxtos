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

/* "sieve" to get only the fourth interrupt, 0x1111 initially */
extern WORD timer_c_sieve;

void init_system_timer(void);
void int_timerc(void);
BOOL timer_test_with_timeout(BOOL (*predicate)(void), LONG delay);

#endif