/*
 * VICKY2 - Functions for VICKY II, the graphics controller of the Foenix A2560U
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

#ifndef VICKY2_H
#define VICKY2_H

#include "config.h"

#if MACHINE_A2560U

#include <stdint.h>
#include "foenix.h"

typedef struct {
    uint8_t  id;
    uint16_t w;
    uint16_t h;
    uint16_t colors;
    uint8_t  fps;
} FOENIX_VIDEO_MODE;

typedef struct __attribute__((__packed__)) {
    uint8_t alpha;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} COLOR32;

#define VICKY_LUT(n) ((COLOR32* volatile)(VICKY_LUTS+n*0x400))

/* Number of colors VICKY can produce */
#define VICKY_PALETTE   (8*8*8) /* 24 bits palette */
#define VRAM0_SIZE      0x200000 /* 2MB */

/* VICKY II */
#define VICKY_CTRL            VICKY

/* Channel A (first screen), VICKY II has only one anyway */
#define VICKY_A_BORDER_CTRL_L (VICKY+0x0004) /* Border control */
#define VICKY_A_BORDER_COLOR  (VICKY+0x0008)
#define VICKY_A_BG_COLOR      (VICKY+0x000C) /* Background control */
#define VICKY_A_BMP_FG_CTRL   (VICKY+0x0100) /* Bitmap layer 0 control */
#define VICKY_A_BMP_FB        (VICKY+0x0104) /* Framebuffer address relative to VRAM */
/* Color palettes */
#define VICKY_NLUTS           8            /* Number of luts */
#define VICKY_LUTS            (VICKY+0x2000) /* Color lookup tables, 0x400 bytes each */
/* Mouse */
#define MOUSE_POINTER_MEMORY  (VICKY+0x0400)
#define MOUSE_POINTER_CTRL    (VICKY+0x0C00)

extern const FOENIX_VIDEO_MODE foenix_video_modes[];

void vicky2_init(void);
void vicky2_set_background_color(uint32_t color);
void vicky2_set_border_color(uint32_t color);
void vicky2_get_video_mode(FOENIX_VIDEO_MODE *result);
void vicky2_set_bitmap0_address(const uint8_t *address); /* address is relative to VRAM */
void vicky2_set_mouse_visible(uint16_t visible);

#endif // MACHINE_A2560U

#endif // VICKY2_H