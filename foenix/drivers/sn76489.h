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

#ifndef SN76489_H
#define SN76489_H

#include <stdint.h>

/* Select the PSG (depends on the machine. On A2560U: 0:left, 1:right, 2:both) */
void sn76489_select(int number);

void sn76489_mute_all(void);
void sn76489_freq(uint8_t voice, uint16_t frequency);
void sn76489_tone(uint8_t voice, uint16_t period);
void sn76489_attenuation(uint8_t voice, uint8_t attenuation);

/*
 * Set the noise source.
 * type: 0:periodid, 1:white
 * source: 0:N/512 1:N/1024 2:N/2048 3:Tone generator 3
 */
void sn76489_noise_source(uint8_t type, uint8_t source);

#endif /* SN76489_H */
