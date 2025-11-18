#ifndef _FNX_KEYBOARD_H_
#define _FNX_KEYBOARD_H_

#include <stdint.h>
#include "a2560_struct.h"


void a2560_kbd_init(const uint32_t *counter, uint16_t counter_freq);

/* Theey return the previous handler so you can daisy chain them */
scancode_handler_t a2560_ps2_set_key_up_handler(scancode_handler_t);
scancode_handler_t a2560_ps2_set_key_down_handler(scancode_handler_t);
mouse_packet_handler_t a2560_ps2_set_mouse_handler(mouse_packet_handler_t);

#endif