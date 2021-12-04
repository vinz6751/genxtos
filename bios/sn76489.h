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

void sn76489_mute_all(void);
void sn76489_tone(uint8_t voice, int frequency);
void sn76489_attenuation(uint8_t voice, uint8_t attenuation);

#endif // SN76489_H
