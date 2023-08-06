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


/* VICKY II */
//#define VICKY_CTRL                  VICKY
    #define VICKY_CTRL_TEXT       0x00000001
    #define VICKY_CTRL_TXTOVERLAY 0x00000002
    #define VICKY_CTRL_GFX        0x00000004
    #define VICKY_CTRL_BITMAP     0x00000008
    #define VICKY_CTRL_TILE       0x00000010
    #define VICKY_CTRL_SPRITE     0x00000020
    #define VICKY_CTRL_DISABLE    0x00000080
    #define VICKY_CTRL_CLK40      0x80000000  /**< Indicate if PLL is 25MHz (0) or 40MHz (1) */    
    #define VICKY_BORDER_HEIGHT   0x003f0000L

/* Channel A (first screen), VICKY II has only one anyway */
//#define VICKY_A_BORDER_CTRL         (VICKY+0x0004) /* Border control */
    #define VICKY_BORDER_ENABLE   0x00000001
    #define VICKY_BORDER_SCROLLX  0x00000070
    #define VICKY_BORDER_WIDTH    0x00003f00L
    #define VICKY_BORDER_HEIGHT   0x003f0000L

//#define VICKY_B_BORDER_COLOR        (VICKY+0x0008) /* Border colour 0x--RRGGBB */
//#define VICKY_A_CURSOR_CTRL         (VICKY+0x0010) /* Cursor configuration */
    #define VICKY_CURSOR_ENABLE     0x00000001
    #define VICKY_CURSOR_RATE       0x00000006
    #define VICKY_CURSOR_CHAR       0x00ff0000L

//#define VICKY_A_CURSOR_POS          (VICKY+0x0014) /* Cursor position */
    #define VICKY_CURSOR_X          0x0000ffff
    #define VICKY_CURSOR_Y          0xffff0000
//#define VICKY_B_BG_COLOR            (VICKY+0x000C) /* Background control */
//#define VICKY_A_BMP0_FG_CTRL        (VICKY+0x0100) /* Bitmap layer 0 control */
//#define VICKY_A_BMP0_FB             (VICKY+0x0104) /* Bitmap layer 0 framebuffer address relative to VRAM */
//#define VICKY_A_BMP1_FG_CTRL        (VICKY+0x0108) /* Bitmap layer 1 control */
//#define VICKY_A_BMP1_FB             (VICKY+0x010c) /* Bitmap layer 1 framebuffer address relative to VRAM */

/* Color palettes */
#define VICKY_NLUTS           8      /* Number of luts */

/* Mouse */
#define VICKY_MOUSE_MEM         (VICKY+0x0400)
#define VICKY_MOUSE_CTRL        (VICKY+0x0C00)
    #define VICKY_MOUSE_ENABLE  1
    #define VICKY_MOUSE_CHOICE  2
#define VICKY_MOUSE_SUPPORT   (VICKY+0x0C02)
#define VICKY_MOUSE_X         (VICKY_MOUSE_SUPPORT)
#define VICKY_MOUSE_Y         (VICKY_MOUSE_SUPPORT+2)
#define VICKY_MOUSE_PACKET    (VICKY_MOUSE_SUPPORT+8) /* 3 words (16bits)/ PS/2 mouse packet that VICKY knows how to interpret */

/* Text memory */
#define VICKY_TEXT_COLOR      (VICKY_FONT) /* Font memory (-> 0xbfff) */
#define VICKY_TEXT_COLOR_FG   (VICKY_TEXT+0xC400) /* Text foreground colours 16x 32bit colors (-> 0x43f) */
#define VICKY_TEXT_COLOR_BG   (VICKY_TEXT+0xC440) /* Text foreground colours 16x 32bit colors (-> 0x47f) */
#define VICKY_TEXT_COLOR_SIZE 32                  /* Size of text palette */

#define VICKY_MODE_MASK       0x00000700L
/* Normal pixels */
#ifdef MACHINE_A2560U
#define VICKY_MODE_640x480_60 0x00
#define VICKY_MODE_800x600_60 0x01
#define VICKY_MODE_RESERVED2  0x02
#define VICKY_MODE_640x400_70 0x03
/* Doubled pixels */
#define VICKY_MODE_320x240_60 0x04
#define VICKY_MODE_400x300_60 0x05
#define VICKY_MODE_RESERVED6  0x06
#define VICKY_MODE_320x200_70 0x07
#elif defined(MACHINE_A2560X) || defined(MACHINE_A2560K)
/* Channel A (text only) */
#define VICKY_A_MODE_800x600_60  0x00
#define VICKY_A_MODE_1024x768_60 0x01
/* Channel B (full featured) */
#define VICKY_B_MODE_640x480_60  0x00
#define VICKY_B_MODE_800x600_60  0x01
#define VICKY_B_MODE_RESERVED2   0x02
#define VICKY_B_MODE_640x400_70  0x03
/* Doubled pixels mode */
#define VICKY_B_MODE_320x240_60  0x04
#define VICKY_B_MODE_400x300_60  0x05
#define VICKY_B_MODE_RESERVED6   0x06
#define VICKY_B_MODE_320x200_70  0x07
#endif


typedef struct {
    uint16_t id;
    uint16_t w;
    uint16_t h;
    uint16_t colors;
    uint8_t  fps;
} FOENIX_VIDEO_MODE;


extern const uint16_t vicky2_default_palette[];

#ifdef MACHINE_A2560U
union vicky2_color_t {
    uint32_t code;
    struct __attribute__((__packed__)) {
        uint8_t blue;
        uint8_t green;
        uint8_t red; 
        uint8_t alpha;
    };
};
#elif defined(MACHINE_A2560X)
union vicky2_color_t {
    uint32_t code;
    struct __attribute__((__packed__)) {
        uint8_t alpha;
        uint8_t red;
        uint8_t green;
        uint8_t blue;
    };
};
#endif
typedef union vicky2_color_t COLOR32;

struct vicky2_layer_t {
    uint32_t control;
    uint8_t  *address; // Relative to VRAM start
};

struct vicky2_bitmap_t {
    struct vicky2_layer_t layer[3]; // Bitmap 0, bitmap 1, collision detection
};

struct vicky2_palette_t {
    union vicky2_color_t color[256];
};

struct vicky2_bmp_palette_t {
    struct vicky2_palette_t lut[VICKY_NLUTS];
};

#ifdef MACHINE_A2560U
// A2560
struct vicky2_t {
    uint32_t control;
    uint32_t border_control;
    uint32_t border_color;
    uint32_t background_color;
    uint32_t cursor_control;
    uint32_t cursor_position;
    uint16_t line_interrupt[4];
    uint16_t reserved[8];
    uint16_t fpga_year;
    uint16_t fpga_month_day;
    uint16_t pcb_revision[2]; // Null-terminated string
    uint16_t fpga_sub_version;
    uint16_t fpga_version;
    uint16_t fpga_part_number_low;
    uint16_t fpga_part_number_high;
};

extern struct vicky2_bitmap_t * const vicky_bitmap;

extern const FOENIX_VIDEO_MODE vicky2_video_modes[];

struct vicky2_channel_t {
    struct vicky2_t *ctrl;
    struct vicky2_bitmap_t *bitmap;
    struct vicky2_tile_t *tile;
    struct vicky2_collition_t *collision;
    uint16_t *mouse_pointer;
    uint16_t *mouse_control;
    FOENIX_VIDEO_MODE *video_modes;
};

struct vicky2_channel_t * const vicky = VICKY;

#elif defined(MACHINE_A2560X) || defined (MACHINE_A2560K)
struct vicky2_mouse_control_t {
    uint16_t control; // READ AS 32BITS !
    uint16_t reserved02;
    uint16_t yx; // Read only
    uint16_t reserved06;

};

struct vicky2_t {
    uint32_t control;
    uint32_t border_control;
    uint32_t border_color;
    uint32_t background_color;
    uint32_t cursor_control;
    uint32_t cursor_position;
    uint32_t line_interrupt_01;
    uint32_t line_interrupt_23;
    uint32_t font_size_control;
    uint32_t font_count_control;
    uint32_t reserved_0028[6];
    uint32_t mouse_graphics[16*16*2];
    uint16_t mouse_control; // Must write as 32bits !
    uint16_t reserved_c02;
    uint32_t mouse_position; // Read only
    uint16_t reserved_c06;
    uint32_t ps2_byte[3]; // Must write as 32 bits. See doc.
};

struct vicky2_text_memory_t {
    uint8_t text[VICKY_TEXT_SIZE];
    uint8_t reserved_4000[0x4000];
    uint8_t color[VICKY_TEXT_SIZE];
    uint8_t reserved_c000[0x400];
    union vicky2_color_t palette_fg[16];
    union vicky2_color_t palette_bg[16];
};

struct vicky2_mouse_gfx_t {
    uint32_t mouse_gfx[8];
};

struct vicky2_sprite_control_t {
    uint16_t control;
    uint16_t address; // Must be on 16bit boundary address
};

struct vicky2_sprites_control_t {
    struct vicky2_sprite_control_t sprite[64];
};

struct __attribute__((__packed__)) vicky2_gamma_lut_memory_t {
    uint8_t blue[256];
    uint8_t green[256];
    uint8_t red[256];
};

struct vicky2_font_memory_t {
    uint8_t mem[0x1000];
};

struct vicky2_channel_t {
    struct vicky2_t *ctrl;
    struct vicky2_bitmap_t *bitmap;
    struct vicky2_tile_t *tile;
    struct vicky2_collision_t *collision;
    struct vicky2_mouse_gfx_t *mouse_gfx;
    struct vicky2_mouse_control_t *mouse_control;
    struct vicky2_sprites_control_t **sprite;
    struct vicky2_bmp_palette_t *lut;
    struct vicky2_gamma_lut_memory_t *gamma;
    struct vicky2_font_memory_t *font_memory;
    struct vicky2_text_memory_t *text_memory;
    const FOENIX_VIDEO_MODE *video_modes; // Array of video modes
};



// Convenience pointers for the default (or only) screen
extern struct vicky2_channel_t vicky2_channel_a;
extern struct vicky2_channel_t vicky2_channel_b;

extern struct vicky2_channel_t * const vicky; // Convenience

#endif

extern uint32_t vicky_vbl_freq; /* VBL frequency */

void vicky2_init(void);
void vicky2_init_channel(struct vicky2_channel_t *vicky);

/* Color management */
void vicky2_set_background_color(struct vicky2_channel_t *vicky, uint32_t color);
void vicky2_set_border_color(struct vicky2_channel_t *vicky, uint32_t color);
void vicky2_set_lut_color(struct vicky2_channel_t *vicky, uint16_t lut, uint16_t number, uint32_t color);
void vicky2_get_lut_color(const struct vicky2_channel_t *vicky, uint16_t lut, uint16_t number, COLOR32 *result);

/* Video mode */
void vicky2_set_video_mode(struct vicky2_channel_t *vicky, uint16_t mode); /* video mode number */
void vicky2_read_video_mode(const struct vicky2_channel_t *vicky, FOENIX_VIDEO_MODE *result);
void vicky2_set_bitmap_address(struct vicky2_channel_t *vicky, uint16_t layer, const uint8_t *address); /* address is relative to VRAM */
uint8_t *vicky2_get_bitmap_address(const struct vicky2_channel_t *vicky, uint16_t layer); /* address is relative to VRAM */

/* Text output support */
void vicky2_set_text_lut(struct vicky2_channel_t *vicky, const uint16_t *fg, const uint16_t *bg);

/* Text cursor support */
void vicky2_show_cursor(struct vicky2_channel_t *vicky);
void vicky2_hide_cursor(struct vicky2_channel_t *vicky);
void vicky2_set_text_cursor_xy(struct vicky2_channel_t *vicky, uint16_t x, uint16_t y);

/* Mouse support: see vicky_mouse.h */

// Utility
uint32_t convert_atari2vicky_color(uint16_t orgb);

#endif // VICKY2_H