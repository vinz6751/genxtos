#ifndef SCREEN_TT_H
#define SCREEN_TT_H

#include "emutos.h"

#if CONF_WITH_TT_SHIFTER

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

#define TT_SHIFTER          0xffff8262L
#define TT_SHIFTER_BITMASK  0x970f      /* valid bits in TT_SHIFTER */

#define TT_PALETTE_REGS     ((volatile UWORD *)0xffff8400UL)
#define TT_PALETTE_BITMASK  0x0fff      /* valid bits in TT_PALETTE_REGS */

#define TT_VRAM_SIZE        153600UL

void initialise_tt_palette(WORD rez);

#endif

#endif