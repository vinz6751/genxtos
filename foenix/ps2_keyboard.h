/* PS/2 keyboard driver 
 * 
 * Authors:
 *	Vincent Barrilliot
 *  Peter J Weingartner
 *
 * This file is distributed under the BSD license
 * See doc/license.txt for details.
 *
 * This is a keyboard driver. It is notified by on_irq when an interrupt occurred
 * for the device. It quickly stores the received data in a buffer and returns.
 * When the OS allows for more expensive processing to occur, to decode the received data,
 * it calles process(). That method then analyses the buffer and fires the key_up / key_down
 * callbacks from the OS.
 */

#ifndef PS2_KEYBOARD_H
#define PS2_KEYBOARD_H

#include "ps2.h"

extern const struct ps2_driver_t ps2_keyboard_driver;

#endif
