/*
 * keyboard.c - Keyboard routines
 *
 * Copyright (C) 2001-2025 The EmuTOS development team
 *
 * Authors:
 *  LVL   Laurent Vogel
 *  MAD   Martin Doering
 *        Robert de Vries (Linux m68k)
 *        Bjoern Brauel   (Linux m68k)
 *        Roman Hodek     (Linux m68k)
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 *
 * Some code I got from Linux m68k, thanks to the authors! (MAD)
 */

#include "emutos.h"
#include "aciavecs.h"
#include "ikbd.h" /* for MOUSE_REL_POS_REPORT */
#include "keyboard.h" /* for shifty */
#include "keyboard_mouse_emulation.h"
#include "portab.h"
#include "scancode.h"

/*
 * the following stores the packet that is passed to mousevec();
 * a non-zero value in mouse_packet[0] indicates we are currently
 * in mouse emulation mode.
 */
static SBYTE mouse_packet[3];         /* the emulated mouse packet, passed to mousevec() */
/* indicates if we're emulating a mouse with the keyboard (alt-arrow, help/undo keys on a ST), 0 (FALSE) means no,
 * otherwise it's a type of report per IKBD protocol */
#define MOUSE_EMULATION_MODE mouse_packet[0]


/*
 * the following is a count of the number of arrow/click keys currently down;
 * it is used in mouse emulation mode.
 */
static WORD mouse_action_key_pressed_count;


static BOOL is_mouse_action_key(WORD scancode);



void mouse_emulation_init(void) {
    MOUSE_EMULATION_MODE = FALSE;    /* not doing mouse emulation */
    mouse_action_key_pressed_count = 0;  /* no arrow keys pressed initially */
}

BOOL mouse_emulation_is_active(void) {
    return MOUSE_EMULATION_MODE;
}

void mouse_emulation_handle_key_pressed(WORD scancode) {
    if (!is_mouse_action_key(scancode))
        return;

    mouse_action_key_pressed_count++;
}

/* To call when a mouse emulation key (arrow/left/right click) is released */
void mouse_emulation_handle_key_released(WORD scancode) {
    if (!is_mouse_action_key(scancode & ~KEY_RELEASED))
        return;

    if (mouse_action_key_pressed_count > 0)
        mouse_action_key_pressed_count--;
 
    handle_mouse_mode(scancode);    /* exit mouse mode if appropriate */
}

/* Repeat the last emulated mouse action */
void mouse_emulation_repeat(void) {
    /* for compatibility with Atari TOS, the mouse does not move while Control
     * is pressed, although the keyboard remains in mouse emulation mode */
    if (!(shifty & MODE_CTRL))
    {
        KDEBUG(("Repeating mouse packet %02x%02x%02x\n",
                (UBYTE)mouse_packet[0],(UBYTE)mouse_packet[1],(UBYTE)mouse_packet[2]));
        call_mousevec(mouse_packet);
    }
}

/* Return true if the scancode corresponds to a key used to simulate mouse action (movement or click) */
static BOOL is_mouse_action_key(WORD scancode) {
    switch (scancode) {
        case KEY_UPARROW:
        case KEY_DNARROW:
        case KEY_LTARROW:
        case KEY_RTARROW:
        case KEY_EMULATE_LEFT_BUTTON:
        case KEY_EMULATE_RIGHT_BUTTON:
            return TRUE;
    }
    return FALSE;
}

/*
 * check if we should switch into or out of mouse emulation mode
 * if so, send the relevant mouse packet
 *
 * returns TRUE iff we're in mouse mode on exit
 */
BOOL handle_mouse_mode(WORD newkey)
{
    /* distance to travel in pixels */
    SBYTE distance;

    /* indicates that we are emulating a button press/release */
    BOOL button = FALSE;

    /*
     * check if we should be in emulation mode or not
     */
    if ((shifty&MODE_ALT) && (mouse_action_key_pressed_count > 0))
    {
        /* we should be, so ensure that mouse_packet is valid */
        if (!MOUSE_EMULATION_MODE)
        {
            KDEBUG(("Entering mouse emulation mode\n"));
            MOUSE_EMULATION_MODE = MOUSE_REL_POS_REPORT;
        }
    } else {
        if (MOUSE_EMULATION_MODE)    /* emulating, need to clean up */
        {
            /* we send a packet with all buttons up & no movement */
            MOUSE_EMULATION_MODE = MOUSE_REL_POS_REPORT;
            mouse_packet[1] = mouse_packet[2] = 0;
            KDEBUG(("Sending mouse packet %02x%02x%02x\n",
                    (UBYTE)mouse_packet[0],(UBYTE)mouse_packet[1],(UBYTE)mouse_packet[2]));
            call_mousevec(mouse_packet);
            KDEBUG(("Exiting mouse emulation mode\n"));
            MOUSE_EMULATION_MODE = FALSE;
        }
        return FALSE;
    }

    /*
     * set movement distance according to the Shift key
     */
    distance = shifty & MODE_SHIFT ? 1 : 8;

    switch(newkey) {
    case KEY_EMULATE_LEFT_BUTTON:
        mouse_packet[0] |= LEFT_BUTTON_DOWN;
        button = TRUE;
        break;
    case KEY_EMULATE_LEFT_BUTTON | KEY_RELEASED:
        mouse_packet[0] &= ~LEFT_BUTTON_DOWN;
        break;
    case KEY_EMULATE_RIGHT_BUTTON:
        mouse_packet[0] |= RIGHT_BUTTON_DOWN;
        button = TRUE;
        break;
    case KEY_EMULATE_RIGHT_BUTTON | KEY_RELEASED:
        mouse_packet[0] &= ~RIGHT_BUTTON_DOWN;
        break;
    case KEY_UPARROW:
        distance = -distance;
        FALLTHROUGH;
    case KEY_DNARROW:
        mouse_packet[1] = 0;        /* Atari TOS only allows one direction at a time */
        mouse_packet[2] = distance;
        break;
    case KEY_LTARROW:
        distance = -distance;
        FALLTHROUGH;
    case KEY_RTARROW:
        mouse_packet[1] = distance;
        mouse_packet[2] = 0;        /* Atari TOS only allows one direction at a time */
        break;
    default:        /* user pressed a modifier: update distances */
        if (mouse_packet[1] < 0)
            mouse_packet[1] = -distance;
        else if (mouse_packet[1])
            mouse_packet[1] = distance;
        if (mouse_packet[2] < 0)
            mouse_packet[2] = -distance;
        else if (mouse_packet[2])
            mouse_packet[2] = distance;
    }

    /*
     * if the packet is for an emulated mouse button press,
     * reset the x,y distance variables
     */
    if (button)
        mouse_packet[1] = mouse_packet[2] = 0;

    /*
     * for compatibility with Atari TOS, the mouse does not move while Control
     * is pressed, although the keyboard remains in mouse emulation mode
     */
    if (!(shifty&MODE_CTRL))
    {
        KDEBUG(("Sending mouse packet %02x%02x%02x\n",
                (UBYTE)mouse_packet[0],(UBYTE)mouse_packet[1],(UBYTE)mouse_packet[2]));
        call_mousevec(mouse_packet);
    }

    return TRUE;
}
