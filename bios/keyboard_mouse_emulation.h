
/*
 * emulated mouse support (alt-arrowkey support)
 */

/*
 * support for mouse emulation:
 *  alt-insert, alt-home => mouse buttons (standard, but Amiga differs, see below)
 *  alt-arrowkeys: mouse movement
 */

#ifndef KEYBOARD_MOUSE_EMULATION_H
#define KEYBOARD_MOUSE_EMULATION_H

#include "emutos.h"

#ifdef MACHINE_AMIGA
#define KEY_EMULATE_LEFT_BUTTON     KEY_DELETE
#define KEY_EMULATE_RIGHT_BUTTON    KEY_HELP
#elif defined(MACHINE_A2560U) || defined(MACHINE_A2560X)
#define KEY_EMULATE_LEFT_BUTTON     KEY_INSERT
#define KEY_EMULATE_RIGHT_BUTTON    KEY_PAGEUP
#else
#define KEY_EMULATE_LEFT_BUTTON     KEY_INSERT
#define KEY_EMULATE_RIGHT_BUTTON    KEY_HOME
#endif

void mouse_emulation_init(void);
void mouse_emulation_repeat(void);
void mouse_emulation_handle_key_pressed(WORD scancode);
void mouse_emulation_handle_key_released(WORD scancode);
BOOL handle_mouse_mode(WORD newkey);
BOOL is_mouse_action_key(WORD scancode);
BOOL mouse_emulation_is_active(void);

#endif
