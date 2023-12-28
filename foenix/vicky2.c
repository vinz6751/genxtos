/*
 * VICKY2 - Functions for VICKY II, the graphics controller of the Foenix A2560U
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
#include "vicky2.h"
#include "a2560u.h"
#include "foenix.h"
#include "regutils.h"

#define TTRGB_BLACK     0x0000          /* TT palette */
#define TTRGB_BLUE      0x000f
#define TTRGB_GREEN     0x00f0
#define TTRGB_CYAN      0x00ff
#define TTRGB_RED       0x0f00
#define TTRGB_MAGENTA   0x0f0f
#define TTRGB_LTGRAY    0x0aaa
#define TTRGB_GRAY      0x0666
#define TTRGB_LTBLUE    0x099f
#define TTRGB_LTGREEN   0x09f9
#define TTRGB_LTCYAN    0x09ff
#define TTRGB_LTRED     0x0f99
#define TTRGB_LTMAGENTA 0x0f9f
#define TTRGB_YELLOW    0x0ff0
#define TTRGB_LTYELLOW  0x0ff9
#define TTRGB_WHITE     0x0fff

#define A2560_BLUE 0x55b

static const uint16_t tt_dflt_palette[] = {
    TTRGB_WHITE, TTRGB_RED, TTRGB_GREEN, TTRGB_YELLOW,
    TTRGB_BLUE, TTRGB_MAGENTA, TTRGB_CYAN, TTRGB_LTGRAY,
    TTRGB_GRAY, TTRGB_LTRED, TTRGB_LTGREEN, TTRGB_LTYELLOW,
    TTRGB_LTBLUE, TTRGB_LTMAGENTA, TTRGB_LTCYAN, /*TTRGB_BLACK*/ A2560_BLUE,
    0x0fff, 0x0eee, 0x0ddd, 0x0ccc, 0x0bbb, 0x0aaa, 0x0999, 0x0888,
    0x0777, 0x0666, 0x0555, 0x0444, 0x0333, 0x0222, 0x0111, 0x0000,
    0x0f00, 0x0f01, 0x0f02, 0x0f03, 0x0f04, 0x0f05, 0x0f06, 0x0f07,
    0x0f08, 0x0f09, 0x0f0a, 0x0f0b, 0x0f0c, 0x0f0d, 0x0f0e, 0x0f0f,
    0x0e0f, 0x0d0f, 0x0c0f, 0x0b0f, 0x0a0f, 0x090f, 0x080f, 0x070f,
    0x060f, 0x050f, 0x040f, 0x030f, 0x020f, 0x010f, 0x000f, 0x001f,
    0x002f, 0x003f, 0x004f, 0x005f, 0x006f, 0x007f, 0x008f, 0x009f,
    0x00af, 0x00bf, 0x00cf, 0x00df, 0x00ef, 0x00ff, 0x00fe, 0x00fd,
    0x00fc, 0x00fb, 0x00fa, 0x00f9, 0x00f8, 0x00f7, 0x00f6, 0x00f5,
    0x00f4, 0x00f3, 0x00f2, 0x00f1, 0x00f0, 0x01f0, 0x02f0, 0x03f0,
    0x04f0, 0x05f0, 0x06f0, 0x07f0, 0x08f0, 0x09f0, 0x0af0, 0x0bf0,
    0x0cf0, 0x0df0, 0x0ef0, 0x0ff0, 0x0fe0, 0x0fd0, 0x0fc0, 0x0fb0,
    0x0fa0, 0x0f90, 0x0f80, 0x0f70, 0x0f60, 0x0f50, 0x0f40, 0x0f30,
    0x0f20, 0x0f10, 0x0b00, 0x0b01, 0x0b02, 0x0b03, 0x0b04, 0x0b05,
    0x0b06, 0x0b07, 0x0b08, 0x0b09, 0x0b0a, 0x0b0b, 0x0a0b, 0x090b,
    0x080b, 0x070b, 0x060b, 0x050b, 0x040b, 0x030b, 0x020b, 0x010b,
    0x000b, 0x001b, 0x002b, 0x003b, 0x004b, 0x005b, 0x006b, 0x007b,
    0x008b, 0x009b, 0x00ab, 0x00bb, 0x00ba, 0x00b9, 0x00b8, 0x00b7,
    0x00b6, 0x00b5, 0x00b4, 0x00b3, 0x00b2, 0x00b1, 0x00b0, 0x01b0,
    0x02b0, 0x03b0, 0x04b0, 0x05b0, 0x06b0, 0x07b0, 0x08b0, 0x09b0,
    0x0ab0, 0x0bb0, 0x0ba0, 0x0b90, 0x0b80, 0x0b70, 0x0b60, 0x0b50,
    0x0b40, 0x0b30, 0x0b20, 0x0b10, 0x0700, 0x0701, 0x0702, 0x0703,
    0x0704, 0x0705, 0x0706, 0x0707, 0x0607, 0x0507, 0x0407, 0x0307,
    0x0207, 0x0107, 0x0007, 0x0017, 0x0027, 0x0037, 0x0047, 0x0057,
    0x0067, 0x0077, 0x0076, 0x0075, 0x0074, 0x0073, 0x0072, 0x0071,
    0x0070, 0x0170, 0x0270, 0x0370, 0x0470, 0x0570, 0x0670, 0x0770,
    0x0760, 0x0750, 0x0740, 0x0730, 0x0720, 0x0710, 0x0400, 0x0401,
    0x0402, 0x0403, 0x0404, 0x0304, 0x0204, 0x0104, 0x0004, 0x0014,
    0x0024, 0x0034, 0x0044, 0x0043, 0x0042, 0x0041, 0x0040, 0x0140,
    0x0240, 0x0340, 0x0440, 0x0430, 0x0420, 0x0410, TTRGB_WHITE, A2560_BLUE
};

const FOENIX_VIDEO_MODE foenix_video_modes[] = {
    { VICKY_MODE_640x480_60, 640, 480, 256, 60 }, /* VICKY_MODE_640x480_60 */
    { VICKY_MODE_800x600_60, 800, 600, 256, 60 }, /* VICKY_MODE_800x600_60 */
    { 0,   0,   0,   0 },  /* Reserved */
    { VICKY_MODE_640x400_70, 640, 400, 256, 70 }, /* VICKY_MODE_640x400_70 */
    { VICKY_MODE_320x240_60, 320, 240, 256, 60 }, /* VICKY_MODE_320x240_60 */
    { VICKY_MODE_400x300_60, 400, 300, 256, 60 }, /* VICKY_MODE_400x300_60 */
    { 0,   0,   0,   0 },  /* Reserved */
    { VICKY_MODE_320x200_70, 320, 200, 256, 70 } /* VICKY_MODE_320x200_70 */
};


uint32_t vicky_vbl_freq; /* VBL frequency */
static void rts(void)
{
}

void vicky2_init(void)
{
    int i;
    uint32_t fb_size;
    FOENIX_VIDEO_MODE mode;
    uint16_t *w;

    /* Enable video and bitmap, 640x480 */
    R32(VICKY_CTRL) = 0;

    vicky_mouse_init((void(*)(const vicky_mouse_event_t*))rts/* No callback by default */, 0L);

    vicky2_set_video_mode(VICKY_MODE_640x480_60);
    vicky2_read_video_mode(&mode);

    /* No border. If we used borders we would have to offset/resize all our graphics stuff */
    /* Use instead this to enable border to you can set its color as debugging trace:
     * R32(VICKY_A_BORDER_CTRL) = 0x00030301;
     */
    R32(VICKY_A_BORDER_CTRL) = 0;

    /* Setup framebuffer to point to beginning of video RAM */
    R32(VICKY_A_BMP0_FG_CTRL) = 1;       /* Enable bitmap layer 0, LUT 0 */
    R32(VICKY_A_BMP0_FB) = 0L;           /* Set framebuffer address (relative to VRAM) */
    //R32(VICKY_A_BG_COLOR) = 0xffffffff; /* White background */
    R32(VICKY_A_BG_COLOR) = 0xff0e2b4f; /* Blue-ish background */

    /* Initialise the LUT0 which we use for the screen */
    for (i = 0; i < 256; i++)
        vicky2_set_lut_color(0, i, convert_atari2vicky_color(tt_dflt_palette[i]));

    /* Clear the screen */
    fb_size = (uint32_t)mode.w * mode.h / sizeof(uint16_t);
    fb_size -= 1;
    w = (uint16_t*)VRAM_Bank0;
    while (fb_size--)
        *w++ = 0;
}

void vicky2_set_background_color(uint32_t color)
{
    R32(VICKY_A_BG_COLOR) = color;
}

void vicky2_set_border_color(uint32_t color)
{
    /* It's important to address these as bytes */
    R32(VICKY_A_BORDER_COLOR) = color;
}

void vicky2_set_lut_color(uint16_t lut, uint16_t number, uint32_t color)
{
    volatile uint8_t * c = (uint8_t*)(0xB42000 + lut*0x400 + number*4);
    COLOR32 *clr = (COLOR32*)&color;
    c[0] = clr->blue; // B
    c[1] = clr->green; // G
    c[2] = clr->red; // R
    c[3] = 0xff; // A
}

void vicky2_get_lut_color(uint16_t lut, uint16_t number, COLOR32 *result)
{
    COLOR32* lut_entry = ((COLOR32*)(VICKY_LUTS+number*0x400));

    result->alpha = lut_entry->alpha;
    result->red = lut_entry->red;
    result->green = lut_entry->green;
    result->blue = lut_entry->blue;
}

void vicky2_read_video_mode(FOENIX_VIDEO_MODE *result)
{
    uint32_t video;
    uint32_t border;

    video = R32(VICKY_CTRL);
    *result = foenix_video_modes[(video & 0x300) >> 8];

    /* Take into account the size of the border */
    border = R32(VICKY_A_BORDER_CTRL) ;
    if (border & VICKY_A_BORDER_ENABLE)
    {
        result->w -= (border & VICKY_A_BORDER_WIDTH) >> 7;
        result->h -= (border & VICKY_A_BORDER_HEIGHT) >> 15;
    }

    /* If pixel doubling, divide resolution by 2 */
    if (video & 0x400)
    {
        result->w /= 2;
        result->h /= 2;
    }
}

void vicky2_set_video_mode(uint16_t mode)
{
    /* In theory we should disable interrupts so nobody changes the
     * VICKY control register while we're messing with it... */
    R32(VICKY_CTRL) = (R32(VICKY_CTRL) & ~VICKY_MODE_MASK) | ((mode & 0x07) << 8);
    vicky_vbl_freq = foenix_video_modes[mode & 0x03].fps;

    a2560u_debugnl("Video mode %d set, new VICKY ctrl: %p", mode, (void*)R32(VICKY_CTRL));
}


static const uint32_t *bmp_layer_control[] = {
    (uint32_t * const)VICKY_A_BMP0_FB,
    (uint32_t * const)VICKY_A_BMP1_FB 
};


void vicky2_set_bitmap_address(uint16_t layer,const uint8_t *address)
{
    R32(bmp_layer_control[layer]) = (uint32_t)address; /* Set framebuffer address (relative to VRAM) */
}


uint8_t *vicky2_get_bitmap_address(uint16_t layer) /* address is relative to VRAM */
{
    return (uint8_t*)R32(bmp_layer_control[layer]);
}

/* Convert an Atari 0x0RGB (4 bytes each) colour to FOENIX format */
uint32_t convert_atari2vicky_color(uint16_t orgb)
{
    COLOR32 dst;

    /* For quick 4bit to 8bit scaling */
    const uint8_t c4to8bits[] = {
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};

    dst.alpha = 0xff;
    dst.red = c4to8bits[(orgb & 0xf00) >> 8];
    dst.green = c4to8bits[(orgb & 0x0f0) >> 4];
    dst.blue = c4to8bits[(orgb & 0x00f)];
    return *((uint32_t*)&dst);
}


void vicky2_set_text_lut(const uint16_t *fg, const uint16_t *bg)
{
    /* Colors are represented as GGBB AARR, 2 words for each palette entry. */
    int i;
    volatile uint16_t * const fglut = (uint16_t*)VICKY_TEXT_COLOR_FG;
    volatile uint16_t * const bglut = (uint16_t*)VICKY_TEXT_COLOR_BG;

    for (i = 0; i < VICKY_TEXT_COLOR_SIZE; i++)
    {
        fglut[i] = fg[i];
        bglut[i] = bg[i];
    }
}


void vicky2_show_cursor(void)
{
    R32(VICKY_A_CURSOR_CTRL) |= VICKY_CURSOR_ENABLE;
}


void vicky2_hide_cursor(void)
{
    R32(VICKY_A_CURSOR_CTRL) &= ~VICKY_CURSOR_ENABLE;
}


void vicky2_set_text_cursor_xy(uint16_t x, uint16_t y)
{
    R32(VICKY_A_CURSOR_POS) = ((uint32_t)x) << 16 | y;
}
