#ifndef SCREEN_ATARI_H
#define SCREEN_ATARI_H

#include "emutos.h"

#if CONF_WITH_ATARI_VIDEO

#define STE_LINE_OFFSET     0xffff820fL /* additional registers in STe */
#define STE_HORZ_SCROLL     0xffff8265L

#define ST_PALETTE_REGS     0xffff8240L

void screen_atari_init_mode(void);

/* Resolution-related */
int shifter_screen_can_change_resolution(void);
WORD shifter_get_monitor_type(void);
void atari_setrez(WORD rez, WORD videlmode);
WORD atari_getrez(void);
void atari_setphys(const UBYTE *addr);
const UBYTE *atari_physbase(void);
WORD atari_check_moderez(WORD moderez);
ULONG atari_calc_vram_size(void);
void atari_get_current_mode_info(UWORD *planes, UWORD *hz_rez, UWORD *vt_rez);

/* Color-palette-related*/
WORD atari_get_palette(void);
WORD atari_setcolor(WORD colorNum, WORD color);
void fixup_ste_palette(WORD rez);
void initialise_ste_palette(UWORD mask);

#endif

#endif
