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


/* Attenuation levels a.k.a A0-A3 (you can ADD/OR them) */
#define SN76489_ATT_2dB  1
#define SN76489_ATT_4dB  2
#define SN76489_ATT_8dB  4
#define SN76489_ATT_16dB 8
#define SN76489_ATT_OFF  0xf

/* Noise feedback */
#define SN76489_NSE_WHITE    4    /* Use OR */
#define SN76489_NSE_PERIODIC (~4) /* Use AND */

/* Noise rate */
#define SN76489_NSE_RATE_512  0
#define SN76489_NSE_RATE_1024 1 /* Doc is wrong ? */
#define SN76489_NSE_RATE_2048 2
#define SN76489_NSE_TONE3     3


void sn76489_select(uint8_t *psg);
void sn76489_mute(void);
void sn76489_tone(uint8_t channel, uint16_t frequency);
void sn76489_noise(uint8_t type, uint16_t rate);
void sn76489_attenuation(uint8_t channel, uint8_t attenuation);
uint16_t sn76489_compute_frequency(uint16_t frequency);

#endif // SN76489_H
