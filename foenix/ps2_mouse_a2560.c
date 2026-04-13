/* ps2_mouse_a2560 - PS/2 mouse driver for A2560 Foenix
 * This is really a bridge between the PS/2, OS and VICKY mouse handling.
 * The PS/2 controller asks this driver if it suppose the mouse (ps2_device_type[0]) and if so,
 * it will call the process() function when a byte is received.
 *
 * Author:
 *	Vincent Barrilliot
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 *
 */

#include <stdint.h>
#include <stddef.h>
#include "ps2.h"
#include "a2560.h"

/* Settings */
#define MAXIMUM_TOS_COMPATIBILITY 0

 /* Prototypes */
static const char driver_name[] = "A2560 PS/2 Mouse";
static bool init(struct ps2_driver_api_t *api);
static bool can_drive(const uint8_t ps2_device_type[]);
static void process(struct ps2_driver_api_t *api, uint8_t byte);

/* The PS/2 driver itself */
const struct ps2_driver_t ps2_mouse_driver_a2560u =
{
    .name = driver_name,
    .init = init,
    .can_drive = can_drive,
    .process = process
};

 /* Important states the mouse state machine can be in */
 #define SM_IDLE 0
 #define SM_RECEIVED 3

struct __attribute__((packed)) ps2_mouse_local_t  {
    uint8_t packet[3];
    uint8_t state;
}; /* Must be 4 bytes maximum  !! */

/* Convenience */
#define DRIVER_DATA ((struct ps2_mouse_local_t*)&(api->driver_data))

static void on_change(const vicky_mouse_event_t *event);


static bool init(struct ps2_driver_api_t *api)
{
    a2560_debugnl("ps2_mouse_a2560->init()");

    vicky_mouse_init(on_change, api);

    return SUCCESS;
}

static bool can_drive(const uint8_t ps2_device_type[])
{
	switch (ps2_device_type[0])
	{
		case 0x00: /* Standard PS/2 mouse */
		case 0x03: /* mouse with scroll wheel */
		case 0x04: /* 5-button mouse */
			return SUCCESS;
		default:
			return ERROR;
	}
}

/* Callback from VICKY mouse driver to notify the PS/2 driver that the mouse has moved. */
static void on_change(const vicky_mouse_event_t *event)
{
    struct ps2_driver_api_t *api = (struct ps2_driver_api_t *)event->user_data;
    /*struct ps2_mouse_local_t *driver_data = (struct ps2_mouse_local_t *)api->driver_data;*/

    /* Emulate a IKBD mouse packet */
    int8_t ikbd_packet[3];

    int8_t *packet = (int8_t*)event->ps2packet;
    ikbd_packet[0] = 0xf8 | (*packet++ & 3);
    ikbd_packet[1] = *packet++;
    /* The mouse moves in the opposite direction of the packet */
    ikbd_packet[2] = -(*packet++);
    api->callbacks.on_mouse(ikbd_packet);
}

/* Process a byte received from the PS/2 port. */
void process(struct ps2_driver_api_t *api, uint8_t byte)
{
#if MAXIMUM_TOS_COMPATIBILITY
    /* This is the slower but more compatible option. We write VICKY PS/2 registers as it's the 
     * only way (currently?) to get the mouse cursor to move. But we simulate an IKBD relative mouse
     * packet and pass it to the "mousevec" vector of the TOS so all TOS hooks etc. can work. */
    
    vicky_mouse_ps2(byte);
#else
    /* Receive a byte from the PS/2 port and store it in the packet. */
    uint8_t *packet = DRIVER_DATA->packet;
    uint8_t *state = &(DRIVER_DATA->state);
    packet[(*state)++] = byte;
    
    /* Packet complete */
    if (*state >= SM_RECEIVED)
    {
        *state = SM_IDLE;
        api->callbacks.on_mouse(packet);
   }
#endif
}
