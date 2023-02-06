/* PS/2 keyboard driver 
 * 
 * Authors:
 *	Vincent Barrilliot
 *  Peter J Weingartner
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 *
 * This is a keyboard driver for PS/2 keyboard.
 */

#include <stdint.h>
#include <stddef.h>
#include "ps2.h"
#include "ps2_scancodes.h"

 /* Prototypes */
static const char driver_name[] = "PS/2 Keyboard";
static bool init(struct ps2_driver_api_t *api);
static void process(const struct ps2_driver_api_t *api, uint8_t scancodestate);
static bool can_drive(const uint8_t ps2_device_type[]);

/* Driver */
const struct ps2_driver_t ps2_keyboard_driver =
{
    .name = driver_name,
    .init = init,
    .can_drive = can_drive,
    .process = process
};

/* States the keyboard state machine can be in */
typedef enum sm_state
{
    SM_IDLE = 0,
    SM_E0,
    SM_E02A,
    SM_E02AE0,
    SM_E1,
    SM_E11D,
    SM_E11D45,
    SM_E11D45E1,
    SM_E11D45E19D,
    SM_E0B7,
    SM_E0B7E0
} sm_state_t;


/* Translation tables */
/* Scancodes for E0xx. The index is xx */
static const uint8_t scancodeSet1_E0_to_key[128] = 
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x00 - 0x07 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x08 - 0x0F */
    KEY_PREVIOUSSONG, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x10 - 0x17 */
    0x00, KEY_NEXTSONG, 0x00, 0x00, KEY_KPENTER, KEY_RIGHTCTRL, 0x00, 0x00, /* 0x18 - 0x1F */
    KEY_MUTE, KEY_CALC, KEY_PLAYCD, 0x00, KEY_STOPCD, 0x00, 0x00, 0x00, /* 0x20 - 0x27 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, KEY_VOLUMEDOWN, 0x00, /* 0x28 - 0x2F */
    KEY_VOLUMEUP, 0x00, KEY_HOMEPAGE, 0x00, 0x00, KEY_KPSLASH, 0x00, 0x00, /* 0x30 - 0x37 */
    KEY_RIGHTALT, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x38 - 0x3F */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, KEY_HOME, /* 0x40 - 0x47 */
    KEY_UP, KEY_PAGEUP, 0x00, KEY_LEFT, 0x00, KEY_RIGHT, 0x00, KEY_END, /* 0x48 - 0x4F */
    KEY_DOWN, KEY_PAGEDOWN, KEY_INSERT, KEY_DELETE, 0x00, 0x00, 0x00, 0x00, /* 0x50 - 0x57 */
    0x00, 0x00, 0x00, KEY_LEFTMETA, KEY_RIGHTMETA, KEY_MENU, KEY_POWER, KEY_SLEEP, /* 0x58 - 0x5F */
    0x00, 0x00, 0x00, KEY_WAKEUP, 0x00, KEY_SEARCH, KEY_BOOKMARKS, KEY_REFRESH, /* 0x60 - 0x67 */
    KEY_STOP, KEY_FORWARD, KEY_BACK, KEY_COMPUTER, KEY_MAIL, KEY_MEDIA, 0x00, 0x00, /* 0x68 - 0x6F */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x70 - 0x77 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  /* 0x78 - 0x7F */
 };

/* Internal state of the driver */
struct ps2_keyboard_local_t
{
    sm_state_t state;  /* State of the state machine */   
};

/* Convenience */
#define DRIVER_DATA ((struct ps2_keyboard_local_t*)api->driver_data)

/* Initialise the driver for operations */
static bool init(struct ps2_driver_api_t *api)
{
	/* Shut down leds */
    api->driver->process = process;
    api->driver_data = (api->malloc)(sizeof(struct ps2_keyboard_local_t));
    if (api->driver_data == NULL)
        return ERROR;

    DRIVER_DATA->state = SM_IDLE;

    return SUCCESS;
}

/* Given a PS/2 device type (as returned by the device), indicate if the driver
 * supports it. */
static bool can_drive(const uint8_t ps2_device_type[])
{
    if (ps2_device_type[0] == 0xAB)
    {
        switch (ps2_device_type[1])
        {
            case 0x41:
            case 0xC1:
                /* MF2 keyboard with translation enabled in the PS/Controller (not possible for the second PS/2 port) */
                return SUCCESS;
            case 0x83:
                /* MF2 keyboard without translation: we don't support (yet?)because we expect scancode set 1 */
            default:
                ;
        }
    }
    return ERROR;
}

static void process(const struct ps2_driver_api_t *api, uint8_t scancode)
{
    /* Factoring helpers, see the meat below */

    #define STATE ((struct ps2_keyboard_local_t*)(api->driver_data))->state
    #define IS_BREAK(x) (x & 0x80)

    /* On unexpected scan code, return the state machine to idle (swallow the code) */
    #define check_and_set_next(expected, next) STATE = (scancode == expected) ? next : SM_IDLE;

    #define check_and_down(expected, pressed) \
        if (scancode == expected) \
            api->os_callbacks.on_key_down(pressed); \
        /* On success we're ideal, on failure we swallow the scan code */ \
        STATE = SM_IDLE;

    #define check_and_up(expected, released) \
        if (scancode == expected) \
            api->os_callbacks.on_key_up(scancode); \
        /* On success we're ideal, on failure we swallow the scan code */ \
        STATE = SM_IDLE;

    /* These are illegal, we just swallow them */
	if (scancode == 0 || scancode == 0x80)    
        return;
    /* Process the byte according to the state machine... */
    switch (STATE) {
        case SM_IDLE:
            switch (scancode) {
                case 0xE0:
                    STATE = SM_E0;
                    break;

                case 0xE1:
                    STATE = SM_E1;
                    break;

                default:
                    if (IS_BREAK(scancode))
                        api->os_callbacks.on_key_up(scancode & 0x7f);
                    else
                        api->os_callbacks.on_key_down(scancode);
                    break;
            }
            break;

        case SM_E0:
            switch (scancode) {
                case 0x2A:
                    STATE = SM_E02A;
                    break;

                case 0xB7:
                    STATE = SM_E0B7;
                    break;

                default:
                    {
                        // Lookup the translation of the E0 prefixed code...
                        uint8_t translated_code = scancodeSet1_E0_to_key[scancode & 0x7f];
                        if (translated_code != 0) {
                            if (IS_BREAK(scancode))
                                api->os_callbacks.on_key_up(translated_code);
                            else
                                api->os_callbacks.on_key_down(translated_code);
                        }
                        STATE = SM_IDLE;
                    }
                    break;
            }
            break;

        case SM_E02A:
            check_and_set_next(0xE0, SM_E02AE0);
            break;
        case SM_E02AE0:
            /* Return our PrintScreen key */
            check_and_down(0x37, KEY_SYSRQ);
            break;

        case SM_E0B7:
            check_and_set_next(0xE0, SM_E0B7E0);
            break;
        case SM_E0B7E0:
            /* Return our PrintScreen break */
            check_and_up(0xAA, KEY_SYSRQ);
            break;

        case SM_E1:
            check_and_set_next(0x1D, SM_E11D);
            break;
        case SM_E11D:
            check_and_set_next(0x45, SM_E11D45);
            break;
        case SM_E11D45:
            check_and_set_next(0xE1, SM_E11D45E1);
            break;
        case SM_E11D45E1:
            check_and_set_next(0x9D, SM_E11D45E19D);
            break;
        case SM_E11D45E19D:
            /* Break key break */
            check_and_up(0xC5, KEY_PAUSE);
            break;

        default:
            /* TODO: kernel panic? ;) */
            ;
    }
}
