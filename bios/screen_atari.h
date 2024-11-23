#ifndef SCREEN_ATARI_H
#define SCREEN_ATARI_H

#include "emutos.h"

#if CONF_WITH_ATARI_VIDEO

#define STE_LINE_OFFSET     0xffff820fL /* additional registers in STe */
#define STE_HORZ_SCROLL     0xffff8265L

#define ST_PALETTE_REGS     0xffff8240L

void initialise_ste_palette(UWORD mask);
void fixup_ste_palette(WORD rez);


#endif

#endif
