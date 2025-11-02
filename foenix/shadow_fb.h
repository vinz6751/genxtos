#ifndef SHADOW_FB_H
#define SHADOW_FB_H

#include <stdint.h>

extern uint8_t  *a2560_sfb_addr;

void a2560_sfb_init(void);
void a2560_sfb_setup(const uint8_t *addr, uint16_t text_cell_height);
void a2560_sfb_mark_screen_dirty(void);
void a2560_sfb_mark_cell_dirty(const uint8_t *cell_address);
void a2560_sfb_copy_fb_to_vram(void);

#endif