/*
 * mform.c - Default mouse cursor
 *
 * Copyright (C) 2019-2022 The EmuTOS development team
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#include "mform.h"

/* Default Mouse Cursor Definition */
static const MFORM arrow_mform = {
    1, 0, 1, 0, 1,
    /* background definition */
    {
        0xE000, /* %1110000000000000 */
        0xF000, /* %1111000000000000 */
        0xF800, /* %1111100000000000 */
        0xFC00, /* %1111110000000000 */
        0xFE00, /* %1111111000000000 */
        0xFF00, /* %1111111100000000 */
        0xFF80, /* %1111111110000000 */
        0xFFC0, /* %1111111111000000 */
        0xFE00, /* %1111111000000000 */
        0xFE00, /* %1111111000000000 */
        0xEF00, /* %1110111100000000 */
        0x0F00, /* %0000111100000000 */
        0x0780, /* %0000011110000000 */
        0x0780, /* %0000011110000000 */
        0x03C0, /* %0000001111000000 */
        0x0000  /* %0000000000000000 */
    },
    /* foreground definition */
    {
        0x4000, /* %0100000000000000 */
        0x6000, /* %0110000000000000 */
        0x7000, /* %0111000000000000 */
        0x7800, /* %0111100000000000 */
        0x7C00, /* %0111110000000000 */
        0x7E00, /* %0111111000000000 */
        0x7F00, /* %0111111100000000 */
        0x7F80, /* %0111111110000000 */
        0x7C00, /* %0111110000000000 */
        0x6C00, /* %0110110000000000 */
        0x4600, /* %0100011000000000 */
        0x0600, /* %0000011000000000 */
        0x0300, /* %0000001100000000 */
        0x0300, /* %0000001100000000 */
        0x0180, /* %0000000110000000 */
        0x0000  /* %0000000000000000 */
    }
};

