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

#define ST_VRAM_SIZE        32000UL
#define FALCON_VRAM_SIZE    368640UL    /* 768x480x256 (including overscan) */

#if CONF_WITH_ATARI_VIDEO

#define VIDEOBASE_ADDR_HI   0xffff8201L
#define VIDEOBASE_ADDR_MID  0xffff8203L
#define VIDEOBASE_ADDR_LOW  0xffff820dL

#define SYNCMODE            0xffff820aL

#define ST_SHIFTER          0xffff8260L

#define SPSHIFT             0xffff8266L


#define FALCON_PALETTE_REGS 0xffff9800L

/* palette color definitions */

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

#endif /* CONF_WITH_ATARI_VIDEO */

/* 16 color palette 0x0RGB format (4 bits per component) */
extern const UWORD default_palette[];

/* Called when we detect that a different monitor is plugged */
void detect_monitor_change(void);

/* set screen address, mode, ... */
void screen_init_address(void);
void screen_init_mode(void);
void screen_init_services(void);
void screen_setphys(const UBYTE *addr);
void screen_set_rez_hacked(void);
void screen_get_current_mode_info(UWORD *planes, UWORD *hz_rez, UWORD *vt_rez);

/* hardware-independent xbios routines */
const UBYTE *physbase(void);
UBYTE *logbase(void);
WORD getrez(void);
WORD setscreen(UBYTE *logLoc, const UBYTE *physLoc, WORD rez, WORD videlmode);
void setpalette(const UWORD *palettePtr);
WORD setcolor(WORD colorNum, WORD color);
void vsync(void);

#endif /* SCREEN_H */
