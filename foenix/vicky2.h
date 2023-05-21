/*
 * VICKY2 - Functions for VICKY II, the graphics controller of the Foenix A2560U
 *
 * Copyright (C) 2021-2022 The EmuTOS development team
 *
 * Authors:
 *  VB   Vincent Barrilliot
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#ifndef VICKY2_H
#define VICKY2_H

#include <stdint.h>
#include "foenix.h"

typedef struct {
    uint16_t id;
    uint16_t w;
    uint16_t h;
    uint16_t colors;
    uint8_t  fps;
} FOENIX_VIDEO_MODE;

typedef struct __attribute__((__packed__)) {
    uint8_t blue;
    uint8_t green;
    uint8_t red; 
    uint8_t alpha;
} COLOR32;

#define VICKY_LUT(n) ((COLOR32* volatile)(VICKY_LUTS+n*0x400))

/* Number of colors VICKY can produce */
#define VICKY_PALETTE   (8*8*8)  /* 24 bits palette */

/* VICKY II */
#define VICKY_CTRL                  VICKY
    #define VICKY_A_CTRL_TEXT       0x00000001
    #define VICKY_A_CTRL_TXTOVERLAY 0x00000002
    #define VICKY_A_CTRL_GFX        0x00000004
    #define VICKY_A_CTRL_BITMAP     0x00000008
    #define VICKY_A_CTRL_TILE       0x00000010
    #define VICKY_A_CTRL_SPRITE     0x00000020
    #define VICKY_A_CTRL_DISABLE    0x00000080
    #define VICKY_A_BORDER_HEIGHT   0x003f0000L

/* Channel A (first screen), VICKY II has only one anyway */
#define VICKY_A_BORDER_CTRL         (VICKY+0x0004) /* Border control */
    #define VICKY_A_BORDER_ENABLE   0x00000001
    #define VICKY_A_BORDER_SCROLLX  0x00000070
    #define VICKY_A_BORDER_WIDTH    0x00003f00L
    #define VICKY_A_BORDER_HEIGHT   0x003f0000L

#define VICKY_A_BORDER_COLOR        (VICKY+0x0008) /* Border colour 0x--RRGGBB */
#define VICKY_A_CURSOR_CTRL         (VICKY+0x0010) /* Cursor configuration */
    #define VICKY_CURSOR_ENABLE     0x00000001
    #define VICKY_CURSOR_RATE       0x00000006
    #define VICKY_CURSOR_CHAR       0x00ff0000L

#define VICKY_A_CURSOR_POS          (VICKY+0x0014) /* Cursor position */
    #define VICKY_CURSOR_X          0x0000ffff
    #define VICKY_CURSOR_Y          0xffff0000
#define VICKY_A_BG_COLOR            (VICKY+0x000C) /* Background control */
#define VICKY_A_BMP0_FG_CTRL        (VICKY+0x0100) /* Bitmap layer 0 control */
#define VICKY_A_BMP0_FB             (VICKY+0x0104) /* Bitmap layer 0 framebuffer address relative to VRAM */
#define VICKY_A_BMP1_FG_CTRL        (VICKY+0x0108) /* Bitmap layer 1 control */
#define VICKY_A_BMP1_FB             (VICKY+0x010c) /* Bitmap layer 1 framebuffer address relative to VRAM */

/* Color palettes */
#define VICKY_NLUTS           8            /* Number of luts */
#define VICKY_LUTS            (VICKY+0x2000) /* Color lookup tables, 0x400 bytes each */
/* Mouse */
#define VICKY_MOUSE_MEM         (VICKY+0x0400)
#define VICKY_MOUSE_CTRL        (VICKY+0x0C00)
    #define VICKY_MOUSE_ENABLE  1
    #define VICKY_MOUSE_CHOICE  2
#define VICKY_MOUSE_SUPPORT   (VICKY+0x0C02)
#define VICKY_MOUSE_X         (VICKY_MOUSE_SUPPORT)
#define VICKY_MOUSE_Y         (VICKY_MOUSE_SUPPORT+2)
#define VICKY_MOUSE_PACKET    (VICKY_MOUSE_SUPPORT+8) /* 3 words (16bits)/ PS/2 mouse packet that VICKY knows how to interpret */


/* Text mode */
#define VICKY_FONT            (VICKY+0x8000)      /* Font memory (-> 0xbff) */
/* Text memory */
#define VICKY_TEXT_COLOR      (VICKY_TEXT+0x8000) /* Font memory (-> 0xbfff) */
#define VICKY_TEXT_COLOR_FG   (VICKY_TEXT+0xC400) /* Text foreground colours 16x 32bit colors (-> 0x43f) */
#define VICKY_TEXT_COLOR_BG   (VICKY_TEXT+0xC440) /* Text foreground colours 16x 32bit colors (-> 0x47f) */
#define VICKY_TEXT_COLOR_SIZE 32                  /* Size of text palette */

extern const FOENIX_VIDEO_MODE foenix_video_modes[];
#define VICKY_MODE_MASK       0x00000700L
/* Normal pixels */
#define VICKY_MODE_640x480_60 0x00
#define VICKY_MODE_800x600_60 0x01
#define VICKY_MODE_RESERVED2  0x02
#define VICKY_MODE_640x400_70 0x03
/* Doubled pixels */
#define VICKY_MODE_320x240_60 0x04
#define VICKY_MODE_400x300_60 0x05
#define VICKY_MODE_RESERVED6  0x06
#define VICKY_MODE_320x200_70 0x07

extern uint32_t vicky_vbl_freq; /* VBL frequency */

void vicky2_init(void);


/* Color management */
void vicky2_set_background_color(uint32_t color);
void vicky2_set_border_color(uint32_t color);
void vicky2_set_lut_color(uint16_t lut, uint16_t number, uint32_t color);
void vicky2_get_lut_color(uint16_t lut, uint16_t number, COLOR32 *result);

/* Video mode */
void vicky2_set_video_mode(uint16_t mode); /* video mode number */
void vicky2_read_video_mode(FOENIX_VIDEO_MODE *result);
void vicky2_set_bitmap_address(uint16_t layer, const uint8_t *address); /* address is relative to VRAM */
uint8_t *vicky2_get_bitmap_address(uint16_t layer); /* address is relative to VRAM */

/* Text output support */
void vicky2_set_text_lut(const uint16_t *fg, const uint16_t *bg);

/* Text cursor support */
void vicky2_show_cursor(void);
void vicky2_hide_cursor(void);
void vicky2_set_text_cursor_xy(uint16_t x, uint16_t y);

/* Mouse support: see vicky_mouse.h */

// Utility
uint32_t convert_atari2vicky_color(uint16_t orgb);

#endif // VICKY2_H