/*
 * SN76489 - Functions for the SN76489 Programmable Sound Generator
 *
 * Copyright (C) 2013-2021 The EmuTOS development team
 *
 * Authors:
 *  VB   Vincent Barrilliot
 *  PJW  Peter Weingartner
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#include "config.h"

#if CONF_WITH_SN76489

#include "sn76489.h"
#include <stdint.h>

# ifdef MACHINE_A2560U
#  include "foenix.h"
# endif

/* Whether to check arguments passed to functions. Safer but slower */
#define SN76489_CHECK_ARGS 0

/* Selected PSG, so we don't have to pass it in each call */
static volatile uint8_t *psg;


void sn76489_select(uint8_t *psg_port)
{
    psg = psg_port;
}


/* Mute all channels (set maximum attenuation) */
void sn76489_mute(void)
{
    int channel;

    /* Set attenuation on all channels to full */
    for (channel = 0; channel < 3; channel++)
        sn76489_attenuation(channel, 15);    
}


/* Set the frequency of one of the three tone channels
 * channel: number of the channel (0 - 2)
 * frequency = the frequency in Hz * 100 */
void sn76489_tone(uint8_t channel, uint16_t frequency)
{
    uint16_t period;

#if SN76489_CHECK_ARGS
    if (channel > 2 || frequency >= 0x400)
        return;
#endif

    period = sn76489_compute_frequency(frequency);

    *psg = (uint8_t)(0x80 | ((channel & 0x03) << 5) | (uint8_t)(period & 0x0f));
    *psg = (uint8_t)((period & 0x3f0) >> 4);
}


/* Set the frequency of one of the three tone channels
 * channel: number of the channel (0 - 2)
 * frequency = the frequency in Hz * 100 */
void sn76489_noise(uint8_t type, uint16_t rate)
{
#if SN76489_CHECK_ARGS
    if ((type & ~4) || (rate & ~3))
        return;
#endif
    *psg = 0xe0 | type | rate;
}


/* Set the attenuation of a channel
 * channel:  = the number of the channel (0 - 3) 3 is the noise generator.
 * attenuation = volume level 0: loudest, 15: silent
 */
void sn76489_attenuation(uint8_t channel, uint8_t attenuation)
{
#if SN76489_CHECK_ARGS
    if ((channel > 3) || (rate & ~0x0f))
        return;
#endif
    *psg = 0x90 | ((channel & 0x03) << 5) | (attenuation & 0x0f);
}


/* Calculate the period to set to achieve a given frequency (10 bits max, ie 0 to 1023)
 * It's recommended to, as much as possible, not call this on demand, but precompute
 * desired frequencies, because we use a division and it's slow. */
uint16_t sn76489_compute_frequency(uint16_t frequency)
{
    return (frequency != 0)
        ? SN76489_CLOCK / (32 * frequency)
        : 0x3ff; /* max value for 10 bits */
}

#endif /* CONF_WITH_SN76489 */