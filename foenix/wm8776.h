/*
 * WM8776 - Functions for the WM8776 Audio codec/mixer
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

#ifndef WM8776_H
#define WM8776_H

#include <stdint.h>

void wm8776_init(void);
void wm8776_send(uint16_t data);
void wm8776_set_digital_volume(uint8_t volume);
uint8_t wm8776_get_digital_volume(void);

#endif // WM8776_H
