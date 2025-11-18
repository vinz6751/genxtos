/*
 * VICKY2 - Functions for VICKY II, the graphics controller of the Foenix A2560U
 *
 * Copyright (C) 2013-2023 The EmuTOS development team
 *
 * Authors:
 *  VB   Vincent Barrilliot
 *  PJW  Peter Weingartner
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#include <stddef.h>
#include <stdint.h>
#include "a2560.h"
#include "cpu.h"
#include "foenix.h"
#include "regutils.h"
#include "vicky2.h"
#include "vicky2_txt_a_logger.h"

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

const uint16_t vicky2_default_palette[] = {
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

#ifdef MACHINE_A2560U
const FOENIX_VIDEO_MODE vicky2_video_modes[] = {
    { VICKY_MODE_640x480_60, 640, 480, 8, 60 }, /* VICKY_MODE_640x480_60 */
    { VICKY_MODE_800x600_60, 800, 600, 8, 60 }, /* VICKY_MODE_800x600_60 */
    { 0,   0,   0,   0 },  /* Reserved */
    { VICKY_MODE_640x400_70, 640, 400, 8, 70 }, /* VICKY_MODE_640x400_70 */
    { VICKY_MODE_320x240_60, 320, 240, 8, 60 }, /* VICKY_MODE_320x240_60 */
    { VICKY_MODE_400x300_60, 400, 300, 8, 60 }, /* VICKY_MODE_400x300_60 */
    { 0,   0,   0,   0 },  /* Reserved */
    { VICKY_MODE_320x200_70, 320, 200, 8, 70 } /* VICKY_MODE_320x200_70 */
};

const struct vicky2_channel_t vicky2_channel = {
    (struct vicky2_t * const )VICKY,
    (struct vicky2_bitmap_t * const )(VICKY+0x100),
    (struct vicky2_tile_t * const)(VICKY+0x200),
    (struct vicky2_collision_t * const )(VICKY + 0x300),
    (struct vicky2_mouse_gfx_t * const)(VICKY + VICKY_MOUSE_MEM_OFFSET), /* Mouse graphics */
    (struct vicky2_mouse_control_t * const)(VICKY + VICKY_MOUSE_CTRL_OFFSET), /* Mouse control */
    (struct vicky2_sprites_control_t ** const)(VICKY + 0x1000),
    (struct vicky2_bmp_palette_t * const)(VICKY + 0x2000),
    (struct vicky2_gamma_lut_memory_t * const)(VICKY + 0x4000),
    (struct vicky2_font_memory_t * const)VICKY_FONT,
    (struct vicky2_text_memory_t * const)VICKY_TEXT,
    vicky2_video_modes
};

/* Default screen */
const struct vicky2_channel_t * const vicky = (const struct vicky2_channel_t * const)&vicky2_channel;

#elif defined (MACHINE_A2560K) || defined(MACHINE_A2560X) || defined(MACHINE_GENX)
const FOENIX_VIDEO_MODE vicky2_a_video_modes[] = {
    { VICKY_A_MODE_800x600_60, 800, 600, 8, 60 }, /* VICKY_MODE_800x600_60 */
    { VICKY_A_MODE_1024x768_60, 1024, 768, 8, 60 }, /* VICKY_MODE_1024x768_60 */
};

const FOENIX_VIDEO_MODE vicky2_b_video_modes[] = {
    { VICKY_B_MODE_640x480_60, 640, 480, 8, 60 }, /* VICKY_MODE_640x480_60 */
    { VICKY_B_MODE_800x600_60, 800, 600, 8, 60 }, /* VICKY_MODE_800x600_60 */
    { 0,   0,   0,   0 },  /* Reserved */
    { VICKY_B_MODE_640x400_70, 640, 400, 8, 70 }, /* VICKY_MODE_640x400_70 */
    { VICKY_B_MODE_320x240_60, 320, 240, 8, 60 }, /* VICKY_MODE_320x240_60 */
    { VICKY_B_MODE_400x300_60, 400, 300, 8, 60 }, /* VICKY_MODE_400x300_60 */
    { 0,   0,   0,   0 },  /* Reserved */
    { VICKY_B_MODE_320x200_70, 320, 200, 8, 70 } /* VICKY_MODE_320x200_70 */
};

const struct vicky2_channel_t vicky2_channel_a = {
    (struct vicky2_t * const)VICKY_A,
    (struct vicky2_bitmap_t * const)0L,
    (struct vicky2_tile_t * const)0L,
    (struct vicky2_collision_t * const)0L,
    (struct vicky2_mouse_gfx_t * const)(VICKY_A + VICKY_MOUSE_MEM_OFFSET), /* Mouse graphics */
    (struct vicky2_mouse_control_t * const)(VICKY_A + VICKY_MOUSE_CTRL_OFFSET), /* Mouse control */
    (struct vicky2_sprites_control_t ** const)0L,
    (struct vicky2_bmp_palette_t * const)0L,
    (struct vicky2_gamma_lut_memory_t * const)0L,
    (struct vicky2_font_memory_t * const)VICKY_FONT_A,
    (struct vicky2_text_memory_t * const)VICKY_TEXT_A,
    vicky2_a_video_modes
};

const struct vicky2_channel_t vicky2_channel_b = {
    (struct vicky2_t * const )VICKY_B,
    (struct vicky2_bitmap_t * const )(VICKY_B+0x100),
    (struct vicky2_tile_t * const)(VICKY_B+0x200),
    (struct vicky2_collision_t * const )(VICKY_B + 0x300),
    (struct vicky2_mouse_gfx_t * const)(VICKY_B + VICKY_MOUSE_MEM_OFFSET), /* Mouse graphics */
    (struct vicky2_mouse_control_t * const)(VICKY_B + VICKY_MOUSE_CTRL_OFFSET), /* Mouse control */
    (struct vicky2_sprites_control_t ** const)(VICKY_B + 0x1000),
    (struct vicky2_bmp_palette_t * const)(VICKY_B + 0x2000),
    (struct vicky2_gamma_lut_memory_t * const)(VICKY_B + 0x4000),
    (struct vicky2_font_memory_t * const)VICKY_FONT_B,
    (struct vicky2_text_memory_t * const)VICKY_TEXT_B,
    vicky2_b_video_modes
};

/* Default screen */
const struct vicky2_channel_t * const vicky = (const struct vicky2_channel_t * const)&vicky2_channel_b;
#elif defined(MACHINE_A2560M)
const FOENIX_VIDEO_MODE vicky3_video_modes[] = {
    { 0, 768, 512, 1, 60 },
    { 1, 768, 512, 8, 60 },
    { 2, 768, 512, 16, 60 },
    { 3, 768, 512, 32, 60 },
    { 4, 1024, 768, 1, 60 },
    { 5, 1024, 768, 8, 60 },
    { 6, 1024, 768, 16, 60 },
    { 7, 1024, 768, 32, 60 }
};

/* A2560M's VICKYII is stripped down, it doesn't have bitmap, border, sprites etc. */
const struct vicky2_channel_t vicky3_channel = {
    (struct vicky2_t * const)VICKY2,
    (struct vicky2_bitmap_t * const)0L,
    (struct vicky2_tile_t * const)0L,
    (struct vicky2_collision_t * const)0L,
    (struct vicky2_mouse_gfx_t * const)(VICKY2 + VICKY_MOUSE_MEM_OFFSET), /* Mouse graphics */
    (struct vicky2_mouse_control_t * const)(VICKY2 + VICKY_MOUSE_CTRL_OFFSET), /* Mouse control */
    (struct vicky2_sprites_control_t ** const)0L,
    (struct vicky2_bmp_palette_t * const)0L,
    (struct vicky2_gamma_lut_memory_t * const)0L,
    (struct vicky2_font_memory_t * const)VICKY_FONT,
    (struct vicky2_text_memory_t * const)VICKY_TEXT,
    vicky3_video_modes
};
const struct vicky2_channel_t * const vicky = (const struct vicky2_channel_t * const)&vicky3_channel;
#else
#error "Cannot figure out which machine to define VICKY"
#endif

/* IRQ handlers */
#if defined(MACHINE_A2560K) || defined(MACHINE_A2560X) || defined(MACHINE_GENX)
void a2560_irq_vicky_a(void); /* VICKY autovector A interrupt handler */
void a2560_irq_vicky_b(void); /* VICKY autovector B interrupt handler */
#elif defined(MACHINE_A2560U) || defined(MACHINE_A2560M)
void a2560_irq_vicky(void); /* Only channel, or channel B on the A2560K/X/GenX */
#else
 #error "Define Foenix machine"
#endif


uint32_t vicky_vbl_freq; /* VBL frequency */
static void rts(void)
{
}

void vicky2_init(void)
{
#if defined(MACHINE_A2560U)
    vicky2_init_channel(&vicky2_channel);
#elif defined (MACHINE_A2560K) || defined(MACHINE_A2560X) || defined(MACHINE_GENX)
    vicky2_init_channel(&vicky2_channel_a);
    vicky2_init_channel(&vicky2_channel_b);
#elif defined(MACHINE_A2560M)
    vicky2_init_channel(vicky);
#endif

#if defined(MACHINE_A2560K) || defined(MACHINE_A2560X) || defined(MACHINE_GENX)
    /* Channel A VBL */
    cpu_set_vector(INT_VICKYII_A, (uint32_t)a2560_irq_vicky_a);
    cpu_set_vector(INT_VICKYII_B, (uint32_t)a2560_irq_vicky_b);
#else
    cpu_set_vector(INT_VICKYII, (uint32_t)a2560_irq_vicky);
#endif
}


void vicky2_init_channel(const struct vicky2_channel_t * const vicky)
{
    int i;
    uint32_t fb_size;
    FOENIX_VIDEO_MODE mode;
    uint16_t *w;
    static const COLOR32 bg_color = {  /* Blue-ish background */
        .alpha = 0xff,
        .red = 0x0e,
        .green = 0x2b,
        .blue = 0x4f
    };
    a2560_debugnl("vicky2_init(%p)", vicky->ctrl);

#if defined(MACHINE_A2560K) || defined(MACHINE_A2560X) || defined(MACHINE_GENX)
    /* Initialise text screen for logging */
    vicky->ctrl->control = VICKY_CTRL_TEXT; /* Default is 800x600 */
    vicky->ctrl->background_color = 0xff8888ff;
    /* EmuTOS code is not ready to support borders */
    /*vicky->ctrl->border_control = 0x00080800|VICKY_BORDER_ENABLE;*/
    vicky->ctrl->border_control = 0;
    vicky->ctrl->border_color = 0xffffffff;
    vicky->ctrl->background_color = 0x00222222; /*convert_atari2vicky_color(vicky2_default_palette[15]);*/
    vicky->ctrl->cursor_control = 0xf0ff0001;
    vicky->ctrl->cursor_position = 0x00000000;
    channel_A_set_fg_color(0);
    channel_A_set_bg_color(15);
    channel_A_cls();
    for (i=0; i<16; i++) {
        vicky->text_memory->palette_bg[i].code = vicky->text_memory->palette_fg[i].code = convert_atari2vicky_color(vicky2_default_palette[i]);
    }
#elif defined(MACHINE_A2560M)
    uint32_t msr_shadow = 0; /* This assumes the default video mode */
    msr_shadow |= VICKY_CTRL_TEXT;

    /* This could probably be moved elsewhere */
#if 0 /* check the offsets */
    a2560_debugnl("vicky->ctrl->control: %p", &vicky->ctrl->control);
    a2560_debugnl("vicky->ctrl->cursor_control: %p", &vicky->ctrl->cursor_control);
    a2560_debugnl("vicky->ctrl->cursor_position: %p", &vicky->ctrl->cursor_position);
    a2560_debugnl("vicky->ctrl->font_size_control: %p", &vicky->ctrl->font_size_control);
    a2560_debugnl("vicky->ctrl->text_window_size: %p", &vicky->ctrl->text_window_size);
    a2560_debugnl("vicky->ctrl->window_pos_x: %p", &vicky->ctrl->window_pos_x);
    a2560_debugnl("vicky->ctrl->window_size_x: %p", &vicky->ctrl->window_size_x);
    a2560_debugnl("vicky->ctrl->window_prefetch: %p", &vicky->ctrl->window_prefetch);
    a2560_debugnl("vicky->text_memory->text: %p", &vicky->text_memory->text);
    a2560_debugnl("vicky->text_memory->color: %p", &vicky->text_memory->color);
#endif

    vicky->ctrl->cursor_control = 0x00410009L; /* Set the Cursor - FONTSet0, SLowest FLash */
    vicky->ctrl->cursor_position = 0;
    vicky->ctrl->font_size_control = 0x08080808L;
    vicky->ctrl->text_window_size = 0x3c50L; /* 60x80 */
    vicky->ctrl->window_pos_x = 0x01ff00B7L;
    //vicky->ctrl->window_size_x = 0x028001E0L;
    vicky->ctrl->window_size_x = 0x028001E0L;
    vicky->ctrl->window_prefetch = 0x01090000L;

#if 0
    uint8_t *t = vicky->text_memory->text;
    uint8_t *c = vicky->text_memory->color;
    #if 0 /* test */
    for (int i=0; i<4800; i++) {
        *t++ = '+';
        *c++ = 0xf2+i;
    }
    #endif
#endif
    vicky->ctrl->control = msr_shadow;
#endif

#if !defined(MACHINE_A2560M) /* Does need it ? */
    /* Kick the PLL
     * If VICKY is generating a 40MHz signal, we need to switch the bit to go to 40MHz before
     * clearing it to go back to 25MHz. Thanks MCP for the trick ! */
    if (vicky->ctrl->control & VICKY_CTRL_CLK40) {
        a2560_debugnl("kicking PLL");
        uint32_t ctrl = vicky->ctrl->control;
        vicky->ctrl->control |= 0x400;
        vicky->ctrl->control &= ~0x400;
        vicky->ctrl->control = ctrl;
    }
    a2560_debugnl("ctrl before setting modes: %p", vicky->ctrl->control);
    /* Enable video and bitmap, 640x480 */
    vicky->ctrl->control = 0;
    a2560_debugnl("ctrl after setting to 0: %p", vicky->ctrl->control);
#endif

    /* TODO: What is this doing here ?? */
    a2560_debugnl("vicky2_init:vicky_mouse_init");
    vicky_mouse_init((void(*)(const vicky_mouse_event_t*))rts/* No callback by default */, 0L);

a2560_debugnl("vicky2_init:vicky2_set_video_mode");
    vicky2_set_video_mode(vicky, 0/*Default mode, actual rez depends on the channel A or B*/);
a2560_debugnl("vicky2_init:vicky2_read_video_mode");
    vicky2_read_video_mode(vicky, &mode);
a2560_debugnl("vicky2_init:done enabling video mode");

#if !defined(MACHINE_A2560M)
    /* No border. If we used borders we would have to offset/resize all our graphics stuff */
    /* Use instead this to enable border to you can set its color as debugging trace:
     * R32(VICKY_A_BORDER_CTRL) = 0x00030301;
     */
    vicky->ctrl->border_control = 0;

    if (vicky->bitmap) {
        a2560_debugnl("Setting up vicky bitmap");
        /* Setup framebuffer to point to beginning of video RAM */
        vicky->bitmap->layer[0].control = 1;        /* Enable bitmap layer 0, LUT 0 */
        vicky->bitmap->layer[0].address = 0L;       /* Set framebuffer address (relative to VRAM) */
        a2560_debugnl("vicky framebuffer: %p", vicky);
        vicky->ctrl->background_color = bg_color.code;

        /* Initialise the LUT0 which we use for the screen */
        for (i = 0; i < 256; i++)
            vicky2_set_lut_color(vicky, 0, i, convert_atari2vicky_color(vicky2_default_palette[i]));
        a2560_debugnl("vicky2_init: lut initialized");
        
        /* Clear the screen */
        fb_size = (uint32_t)mode.w * mode.h / sizeof(uint16_t);
        fb_size -= 1;
        w = (uint16_t*)VRAM_Bank0;
        a2560_debugnl("vicky2_screen @ %p cleared", VRAM_Bank0);
        while (fb_size--)
            *w++ = 0;
    }
#endif

a2560_debugnl("vicky2_init:done");
}

#if !defined(MACHINE_A2560M)

void vicky2_set_background_color(const struct vicky2_channel_t * const vicky, uint32_t color)
{
    vicky->ctrl->background_color = color;
}

void vicky2_set_border_color(const struct vicky2_channel_t * const vicky, uint32_t color)
{
    /* It's important to address these as bytes */
    vicky->ctrl->border_color = color;
}

#endif


void vicky2_set_lut_color(const struct vicky2_channel_t * const vicky, uint16_t lut, uint16_t number, uint32_t color)
{
#ifdef MACHINE_A2560U
    volatile uint8_t * c = (uint8_t *)&vicky->lut->lut[lut].color[number].code;
    COLOR32 *clr = (COLOR32*)&color;

    c[0] = clr->blue; /* B */
    c[1] = clr->green; /* G */
    c[2] = clr->red; /* R */
    c[3] = 0xff; /* A */
#elif defined(MACHINE_A2560K) || defined(MACHINE_A2560M) || defined(MACHINE_A2560X) || defined(MACHINE_GENX)
    vicky->lut->lut[lut].color[number] = *((COLOR32*)&color);
#endif
}

void vicky2_get_lut_color(const struct vicky2_channel_t * const vicky, uint16_t lut, uint16_t number, COLOR32 *result)
{
    *result = vicky->lut->lut[lut].color[number];
}

void vicky2_read_video_mode(const struct vicky2_channel_t * const vicky, FOENIX_VIDEO_MODE *result)
{
    uint32_t video;
    uint32_t border;

    video = vicky->ctrl->control;
#if defined(MACHINE_A2560M)
    *result = vicky->video_modes[(video & VICKY_MODE_MASK) >> 1];
    a2560_debugnl("vicky2_read_video_mode (%p) returning %p, %dx%d", video, result, result->w, result->h);
#else
    *result = vicky->video_modes[(video & 0xd) >> 1];

    /* Take into account the size of the border */
    border = vicky->ctrl->border_control;
    if (border & VICKY_BORDER_ENABLE)
    {
        result->w -= (border & VICKY_BORDER_WIDTH) >> 7;
        result->h -= (border & VICKY_BORDER_HEIGHT) >> 15;
    }

    /* If pixel doubling, divide resolution by 2 */
    if (video & 0x400)
    {
        result->w /= 2;
        result->h /= 2;
    }
#endif
}

void vicky2_set_video_mode(const struct vicky2_channel_t * const vicky, uint16_t mode)
{
    a2560_debugnl("Setting for vicky %p, old ctrl = %p", vicky->ctrl, vicky->ctrl->control);
#if defined(MACHINE_A2560M)
    vicky->ctrl->control = (vicky->ctrl->control & ~VICKY_MODE_MASK) | ((mode & 0x07) << 1);
    vicky_vbl_freq = vicky->video_modes[mode & 0x7].fps;
#else
    /* In theory we should disable interrupts so nobody changes the
     * VICKY control register while we're messing with it... */    
    vicky->ctrl->control = (vicky->ctrl->control & ~VICKY_MODE_MASK) | ((mode & 0x07) << 8);
    vicky_vbl_freq = vicky->video_modes[mode & 0x03].fps;
#endif
    a2560_debugnl("Setting video mode %d (%d,%d), new VICKY ctrl: %p", mode, vicky->video_modes[mode].w, vicky->video_modes[mode].h, (void*)vicky->ctrl->control);
}


void vicky2_set_bitmap_address(const struct vicky2_channel_t * const vicky, uint16_t layer,const uint8_t *address)
{
    if (layer <= 2)
        vicky->bitmap->layer[layer].address = (uint8_t*)address;
}


uint8_t *vicky2_get_bitmap_address(const struct vicky2_channel_t * const vicky, uint16_t layer) /* address is relative to VRAM */
{
    if (layer <= 2)
        return vicky->bitmap->layer[layer].address;
    else
        return (uint8_t*)-1;
}

/* Convert an Atari 0x0RGB (4 bytes each) colour to FOENIX format */
uint32_t convert_atari2vicky_color(uint16_t orgb)
{
    COLOR32 dst;

    /* For quick 4bit to 8bit scaling */
    static const uint8_t c4to8bits[] = {
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};

    dst.alpha = 0xff;
    dst.red = c4to8bits[(orgb & 0xf00) >> 8];
    dst.green = c4to8bits[(orgb & 0x0f0) >> 4];
    dst.blue = c4to8bits[(orgb & 0x00f)];
    return *((uint32_t*)&dst);
}

void printat(const struct vicky2_channel_t * const vicky, int x, int y, char c);
void printat(const struct vicky2_channel_t * const vicky, int x, int y, char c) {
    vicky->text_memory->text[y*77+x] = c;
    vicky->text_memory->color[y*77+x] = 0x01;
}

void vicky2_set_text_lut(const struct vicky2_channel_t * const vicky, const uint16_t *fg, const uint16_t *bg)
{
    int i;
#ifdef MACHINE_A2560U
    /* Colors are represented as GGBB AARR, 2 words for each palette entry. */
    #define TLUTREGSIZE uint16_t
    volatile TLUTREGSIZE * const fglut = (TLUTREGSIZE*)VICKY_TEXT_COLOR_FG;
    volatile TLUTREGSIZE * const bglut = (TLUTREGSIZE*)VICKY_TEXT_COLOR_BG;
    for (i = 0; i < VICKY_TEXT_COLOR_SIZE; i++)
    {
        fglut[i] = fg[i];
        bglut[i] = bg[i];
    }    
#elif defined(MACHINE_A2560K) || defined(MACHINE_A2560M) || defined(MACHINE_A2560X) || defined(MACHINE_GENX)
    #define TLUTREGSIZE uint32_t
    volatile uint32_t * const fglut = (uint32_t*)vicky->text_memory->palette_fg;
    volatile uint32_t * const bglut = (uint32_t*)vicky->text_memory->palette_bg;
    uint32_t * fg32 = (uint32_t *)fg;
    uint32_t * bg32 = (uint32_t *)bg;
    for (i = 0; i < VICKY_TEXT_COLOR_SIZE; i++)
    {
        fglut[i] = (fg32[i] << 16) | (fg32[i] >> 16 & 0xffff);
        bglut[i] = (bg32[i] << 16) | (bg32[i] >> 16 & 0xffff);
    }

    a2560_debugnl("TEXT color lut set fg:%p bg:%p", fglut, bglut);
#endif

#if 0 /* test */

    for (i=0;i<1000;i++) {
        vicky->text_memory->color[i] = (i%0xf); /*| (i % 0xf0) << 4;*/
        vicky->text_memory->text[i] = '0' + i%10;
    }

    static const char message[] = "ceci est un message";
    char *a = (char*)message;
    i = 0;
    while (a[i]) {
        printat(vicky, 20+i, 30, a[i]);
        i++;
    }
#endif
}


void vicky2_show_cursor(const struct vicky2_channel_t *const vicky)
{
    vicky->ctrl->cursor_control |= VICKY_CURSOR_ENABLE;
}


void vicky2_hide_cursor(const struct vicky2_channel_t * const vicky)
{
    vicky->ctrl->cursor_control  &= ~VICKY_CURSOR_ENABLE;
}


void vicky2_set_text_cursor_xy(const struct vicky2_channel_t * const vicky, uint16_t x, uint16_t y)
{
    vicky->ctrl->cursor_position  = ((uint32_t)x) << 16 | y;
}
