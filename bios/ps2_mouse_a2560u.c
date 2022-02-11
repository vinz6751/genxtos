/* ps2_mouse_a2560u - PS/2 mouse driver for A2560U Foenix
 * 
 * Authors:
 *	Vincent Barrilliot
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 *
 */

 #include <stdint.h>
 #include <stddef.h>
 #include "ps2.h"
 #include "lineavars.h"
 #include "a2560u.h"


 /* Prototypes */
static const char driver_name[] = "PS/2 Mouse";
static bool init(struct ps2_driver_api_t *api);
static bool can_drive(const uint8_t ps2_device_type[]);
static void process(const struct ps2_driver_api_t *api, uint8_t byte);

/* Driver itself */
const struct ps2_driver_t ps2_mouse_driver_a2560u =
{
    .name = driver_name,
    .init = init,
    .can_drive = can_drive,
    .process = process
};

 /* Important states the mouse state machine can be in */
#define SM_IDLE 0
#define SM_BYTE0_RECEIVED 1
#define SM_BYTE1_RECEIVED 2
#define SM_BYTE2_RECEIVED 3
#define SM_RECEIVED 3


 /* Internal state of the driver */
struct ps2_mouse_local_t
{
    uint16_t state;  /* State of the state machine (really counts bytes in the packet) */
    uint16_t prev_x;
    uint16_t prev_y;
    uint16_t buttons;
    int8_t   ps2packet[3];
};


/* Convenience */
#define DRIVER_DATA ((struct ps2_mouse_local_t*)api->driver_data)
#define VICKY_MOUSE ((volatile struct vicky_mouse_t *const)VICKY_MOUSE_X)


static bool init(struct ps2_driver_api_t *api)
{
    a2560u_debug("ps2_mouse_a2560u->init()");
    api->driver->process = process;

    api->driver_data = (api->malloc)(sizeof(struct ps2_mouse_local_t));
    if (api->driver_data == NULL)
        return ERROR;

    DRIVER_DATA->state = SM_IDLE;
#if 0    
    DRIVER_DATA->prev_x = VICKY_MOUSE->x = GCURX;
    DRIVER_DATA->prev_y = VICKY_MOUSE->y = GCURY;
#endif    
    DRIVER_DATA->buttons = 0;

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


/* WARNING there are some direct access stuff here because we want this to perform.
 * Remember we can have 9600 interrupts per second... */
void process(const struct ps2_driver_api_t *api, uint8_t byte)
{
    a2560u_debug("%02x", byte);
    if (DRIVER_DATA->state == SM_IDLE && (byte & 0x08) != 0x08)
        return; /* We're lost. Ignore the packet. This is not even safe because if the packet contains a 08 we're fooled. */

#if 0
    DRIVER_DATA->ps2packet[DRIVER_DATA->state++] = (int8_t)byte;
    if (DRIVER_DATA->state == SM_RECEIVED)
    {        
        /* Emulate an IKBD mouse packet. */
        int8_t *packet = (int8_t*)(DRIVER_DATA->ps2packet); /* Tweaking the PS/2 packet into a IKBD one saves us from copying... */
        packet[0] = 0xf8 | (DRIVER_DATA->ps2packet[0] & 3); /* Relative mouse report + buttons */
        // TODO: manage overflow

        api->os_callbacks.on_mouse(packet);
        DRIVER_DATA->state = SM_IDLE;
    } 
#else
    volatile uint16_t * const packet = (uint16_t*)VICKY_MOUSE_PACKET;
    packet[DRIVER_DATA->state] = (uint16_t)byte;
    DRIVER_DATA->state++;
    if (DRIVER_DATA->state >= SM_RECEIVED)
    {
        a2560u_debug(".", byte);
        GCURX = R16(VICKY_MOUSE_X);
        GCURY = R16(VICKY_MOUSE_Y);
#if 0        
        /* Emulate an IKBD mouse packet. */
        uint8_t packet[3];

        packet[0] = 0x08 | DRIVER_DATA->buttons;
        /* VICKY has done the magic, only need to send the info to the OS */        
        packet[1] = VICKY_MOUSE->x - DRIVER_DATA->prev_x;
        packet[2] = VICKY_MOUSE->y - DRIVER_DATA->prev_y;

        a2560u_debug("%02x %02x %02x", packet[0], packet[1], packet[2]);
        //api->os_callbacks.on_mouse(packet);

        DRIVER_DATA->prev_x = VICKY_MOUSE->x;
        DRIVER_DATA->prev_y = VICKY_MOUSE->y;
        /* This is deliberately done last so that in case of overruns we ignore the incoming packet */
#endif
        DRIVER_DATA->state = SM_IDLE;
    }
#endif
}