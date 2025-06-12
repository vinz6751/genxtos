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

#include "config.h"

#if CONF_WITH_MFP
# include "biosdefs.h"
# include "mfp.h"
#elif defined(MACHINE_A2560U) || defined(MACHINE_A2560X)
# include "a2560u_bios.h"
#endif

#include "ikbd.h"
#include "keyboard.h" /* for key_repeat_tick */
#include "sound.h"
#include "timer.h"

/* Non-Atari hardware vectors */
#if !CONF_WITH_MFP
void (*vector_5ms)(void);       /* 200 Hz system timer */
#endif

/*
 * "sieve", to get only the fourth interrupt.  because this is
 * a global variable, it is automatically initialised to zero.
 */
WORD timer_c_sieve;


// Called from timer_S.s 200Hz timer irq handler
void timer_20ms_routine(void);


// TODO: have a list like the VBL list so users can hook into without having to
// hook the full timer C vector, so here wouldn't have to depend on the GEM etv_timer as
// this is a layering breakage.
void timer_20ms_routine(void) {
    // Repeat keys
    key_repeat_tick();

#if CONF_WITH_YM2149
    // Play Dosound() if appropriate
    sndirq();
#endif

    // GEM
    (*etv_timer)(timer_ms); // We may as well hardcode 20...
}


/* The system has a 200Hz timer (every 5ms)
 * At 200Hz, we only increment a counter
 * every 4th tick (ie 50Hz) we do more things */
void init_system_timer(void)
{
    /* The system timer is initially disabled since the sieve is zero (see note above) */
    timer_ms = 20;

#if !CONF_WITH_MFP
    vector_5ms = int_timerc;
#endif

#if CONF_COLDFIRE_TIMER_C
    coldfire_init_system_timer();
#elif defined(MACHINE_LISA)
    lisa_init_system_timer();
#elif CONF_WITH_MFP
    /* Timer C: ctrl = divide 64, data = 192 */
    xbtimer(2, 0x50, 192, (LONG)int_timerc);
#elif defined(MACHINE_A2560U) || defined(MACHINE_A2560X)
    a2560_set_timer(HZ200_TIMER_NUMBER, 200, true, int_timerc);
#endif

    /* On Atari hardware, the timer will really be enabled when sr is set to 0x2500 or lower
     * (ie when MFP interrupts can be picked bu the processor). */
}

void timer_start_20ms_routine(void)
{
    timer_c_sieve = 0b0001000100010001;
}

#if CONF_WITH_MFP
// Use for the Xbtimer on Atari hardware
static const WORD timer_num[] = { MFP_TIMERA, MFP_TIMERB, MFP_200HZ, MFP_TIMERD };
#endif

// XBIOS function
void xbtimer(WORD timer, WORD control, WORD data, LONG vector)
{
    if (timer < 0 || timer > 3)
        return;

#if CONF_WITH_MFP
    mfp_setup_timer(MFP_BASE,timer, control, data);
    mfpint(timer_num[timer], vector);
#elif defined(MACHINE_A2560U) || defined(MACHINE_A2560X)
    a2560_bios_xbtimer(timer, control, data, (void*)vector);
#endif
}

// Returns FALSE is the predicate didn't return true before the timeout (in 200Hz ticks) */
BOOL timer_test_with_timeout(BOOL (*predicate)(void), LONG delay) {
    LONG next = hz_200 + delay;

    while (hz_200 < next) {
        if (predicate()) {
            return TRUE;
        }
    }

    return FALSE; // Timeout
}