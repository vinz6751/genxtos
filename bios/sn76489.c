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

/*
 * Mute all voices on the PSG
 */
void sn76489_mute_all(void) {
    int voice;

    /* Set attenuation on all voices to full */
    for (voice = 0; voice < 3; voice++)
        sn76489_attenuation(voice, 15);    
}

/*
 * Set the frequency of one of the three tone voices
 *
 * Inputs:
 * voice = the number of the voice (0 - 2)
 * frequency = the frequency in Hz * 100
 */
void sn76489_tone(uint8_t voice, int frequency) {
    int n = 0;

    if (voice >= 3)
        return;
        
    if (frequency != 0)
        n = 357954500 / (32 * frequency);

    R8(SN76489_PORT) = 0x80 | ((voice & 0x03) << 5) | (n & 0x0f);
    R8(SN76489_PORT) = (n & 0x3f0) >> 4;
}

/*
 * Set the attenuation of a voice
 *
 * Inputs:
 * voice = the number of the voice (0 - 3)
 * attenuation = volume level 0: loudest, 15: silent
 */
void sn76489_attenuation(uint8_t voice, uint8_t attenuation) {
    R8(SN76489_PORT) = 0x90 | ((voice & 0x03) << 5) | (attenuation & 0x0f);
}

#endif /* CONF_WITH_SN76489 */