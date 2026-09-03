/*
 * screen.h - low-level screen routines
 *
 * Copyright (C) 2001-2022 The EmuTOS development team
 *
 * Authors:
 *  LVL   Laurent Vogel
 *  THH   Thomas Huth
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#ifndef SCREEN_H
#define SCREEN_H

#include "emutos.h"

/* Standard palette colors (RGB 4 bits each) */

#define RGB_BLACK     0x0000            /* ST(e) palette */
#define RGB_BLUE      0x000f
#define RGB_GREEN     0x00f0
#define RGB_CYAN      0x00ff
#define RGB_RED       0x0f00
#define RGB_MAGENTA   0x0f0f
#define RGB_LTGRAY    0x0555
#define RGB_GRAY      0x0333
#define RGB_LTBLUE    0x033f
#define RGB_LTGREEN   0x03f3
#define RGB_LTCYAN    0x03ff
#define RGB_LTRED     0x0f33
#define RGB_LTMAGENTA 0x0f3f
#define RGB_YELLOW    0x0ff0
#define RGB_LTYELLOW  0x0ff3
#define RGB_WHITE     0x0fff

typedef struct {
    void (FP_CALLBACK *init)(void); /* Most basic setup */
    ULONG (FP_CALLBACK *calc_vram_size)(void); /* Calculate the size of the video ram */
    WORD (FP_CALLBACK *check_moderez)(WORD moderez);
    void (FP_CALLBACK *initialise_palette_registers)(WORD rez, WORD mode);
    WORD (FP_CALLBACK *can_change_resolution)(void);
    void (FP_CALLBACK *get_current_mode_info)(UWORD *planes, UWORD *width, UWORD *height);
    void (FP_CALLBACK *setphys)(const UBYTE *addr);
    WORD (FP_CALLBACK *get_monitor_type)(void);
    WORD (FP_CALLBACK *get_number_of_colors_nuances)(void);
    void (FP_CALLBACK *get_pixel_size)(WORD *width,WORD *height);
    UBYTE *(FP_CALLBACK *physbase)(void);
    /* Try behave like SetScreen but may behave video-hardware specific based on rez/videomode (as CT60/Milan overloads of this) */
    WORD (FP_CALLBACK *setscreen)(UBYTE *logical, const UBYTE *physical, WORD rez, WORD videlmode);
    WORD (FP_CALLBACK *setcolor)(WORD colorNum, WORD color);
    void (FP_CALLBACK *set_palette)(const UWORD *new_palette); /* Colors are 4bits (Atari STe-compatible .... rRRR gGGG bBBB  ) */
} SCREEN_DRIVER;

/* This flag indicate that an emulator (like hatari or STEEm) have manipulated the Line-A variables to simulate a bigger screen than what the Atari hardware can support */
extern char rez_was_hacked;

/* Called when we detect that a different monitor is plugged */
void screen_detect_monitor_change(void);

/* Setup some sensible screen defaults */
void screen_init(void);
void screen_init_services_from_mode_info(void);
void screen_setphys(const UBYTE *addr);
void screen_set_rez_hacked(void);
void screen_do_set_palette(const UWORD *new_palette);

/* This factors some code for screen drivers */
void get_std_pixel_size(WORD *width,WORD *height);

/* hardware-independent xbios routines */
const UBYTE * ARGS_ON_STACK physbase(void);
UBYTE * ARGS_ON_STACK logbase(void);
WORD ARGS_ON_STACK getrez(void);
WORD ARGS_ON_STACK setscreen(UBYTE *logLoc, const UBYTE *physLoc, WORD rez, WORD videlmode);
void ARGS_ON_STACK setpalette(const UWORD *palettePtr);
WORD ARGS_ON_STACK setcolor(WORD colorNum, WORD color);
void ARGS_ON_STACK vsync(void);

#endif /* SCREEN_H */
