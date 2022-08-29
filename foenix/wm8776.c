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

#include <stdint.h>
#include "foenix.h"
#include "wm8776.h"


/* We keep track of the last volume set */
static uint8_t wm8776_volume;

/* Forward declarations */
static void wm8776_wait(void);

/* Convenience */
#define DECLARE_codec volatile uint16_t *codec = WM8776_PORT


/* Initialisation data for the WM8776 CODEC
 * R13 - Turn On Headphones
 * R21 - Enable All the Analog In
 * R17, R22, R10, R11, R12 */
static uint16_t wm8776_init_data[] = { 0x1A00,0x2A1F,0x2301,0x2C07,0x1402,0x1602,0x1845 };

void wm8776_init(void)
{        
    int i;
    for (i = 0; i < sizeof(wm8776_init_data)/sizeof(uint16_t); i++)
        wm8776_send(wm8776_init_data[i]);
}


void wm8776_send(uint16_t data)
{
    DECLARE_codec;
    
    wm8776_wait();
    *codec = data;
}


void wm8776_set_digital_volume(uint8_t volume)
{    
    wm8776_volume = volume;
    wm8776_send(0x0A00 | (0xFF - (volume & 0xFF)));
    wm8776_send(0x0400 | ((volume >> 1) & 0xff));
}


uint8_t wm8776_get_digital_volume(void)
{
    return wm8776_volume;
}


static void wm8776_wait(void)
{
    DECLARE_codec;
    while (*codec & 0x8000)
        ;
}
