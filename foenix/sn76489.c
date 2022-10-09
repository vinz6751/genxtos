/*
 * SN76489 - Functions for the SN76489 Programmable Sound Generator
 *
 * Copyright (C) 2013-2022 The EmuTOS development team
 *
 * Authors:
 *  VB   Vincent Barrilliot
 *  PJW  Peter Weingartner
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#include <stdint.h>
#include "sn76489.h"
#include "foenix.h"
#include "regutils.h"


static const uint8_t* psgs_ports[] = {
    SN76489_L, SN76489_R, SN76489_BOTH
};

static uint8_t *sn76489_current;

/*
 * Selects the PSG that next function calls will operate on.
 * 0 : left, 1: right, 2: both
 */
void sn76489_select(int number) {
# ifdef MACHINE_A2560U
    if (number <= SN76489_COUNT)
        sn76489_current = psgs_ports[number];
# endif
}

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
 * Set the frequency of one of the three tone voices.
 * This is really a convenience method.
 *
 * Inputs:
 * voice = the number of the voice (0 - 2)
 * frequency = the desired frequency in Hz
 */
void sn76489_freq(uint8_t voice, uint16_t frequency) {
    long period;
    long f = frequency;

    if (f <<= 5) /* f *= 32, but if frequency if null avoid division by 0 */
        period = SN76489_CLOCK / f;
    else
        period = 0;

    sn76489_tone(voice, period);
}

/*
 * Set the period of one of the three tone voices
 * Inputs:
 * voice = the number of the voice (0 - 2). 
 * period = the period (10bits resolution, ie 0-1023)
 */
void sn76489_tone(uint8_t voice, uint16_t period) {
    R8(sn76489_current) = 0x80 | ((voice & 0x03) << 5) | (period & 0x0f);
    R8(sn76489_current) = (period & 0x3f0) >> 4;
}

/*
 * Set the attenuation of a voice or noise generator
 *
 * Inputs:
 * voice = the number of the voice (0 - 2, 3 is the noise generator)
 * attenuation = volume level 0: loudest, 15: silent
 */
void sn76489_attenuation(uint8_t voice, uint8_t attenuation) {
    R8(sn76489_current) = 0x90 | ((voice & 0x03) << 5) | (attenuation & 0x0f);
}

/*
 * Set the noise source.
 * type: 0:periodid, 1:white
 * source: 0:N/512 1:N/1024 2:N/2048 3:Tone generator 3
 */
void sn76489_noise_source(uint8_t type, uint8_t source) {
    uint8_t v = 0xe0;

    if (type)
        v |= 0x04;
    v |= source & 3;
    R8(sn76489_current) = v;
}
