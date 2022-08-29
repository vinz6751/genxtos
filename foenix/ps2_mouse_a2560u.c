/* ps2_mouse_a2560u - PS/2 mouse driver for A2560U Foenix
 * 
 * Author:
 *	Vincent Barrilliot
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 *
 */

#ifdef MACHINE_A2560U

 #include <stdint.h>
 #include <stddef.h>
 #include "ps2.h"
 #include "lineavars.h"
 #include "a2560u.h"

/* Settings */
#define MAXIMUM_TOS_COMPATIBILITY 0

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
    int8_t   ps2packet[3];
};


/* Convenience */
#define DRIVER_DATA ((struct ps2_mouse_local_t*)api->driver_data)
#define VICKY_MOUSE ((volatile struct vicky_mouse_t *const)VICKY_MOUSE_X)


static bool init(struct ps2_driver_api_t *api)
{
    a2560u_debugnl("ps2_mouse_a2560u->init()");
    api->driver->process = process;

    api->driver_data = (api->malloc)(sizeof(struct ps2_mouse_local_t));
    if (api->driver_data == NULL)
        return ERROR;

    DRIVER_DATA->state = SM_IDLE;

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


void process(const struct ps2_driver_api_t *api, uint8_t byte)
{
    if (DRIVER_DATA->state == SM_IDLE && (byte & 0x08) != 0x08)
        return; /* We're lost. Ignore the packet. This is not even safe because if the packet contains a 08 we're fooled. */

#if MAXIMUM_TOS_COMPATIBILITY
    // This is the slower but more compatible option. We write VICKY PS/2 registers as it's the 
    // only way (currently?) to get the mouse cursor to move. But we simulate an IKBD relative mouse
    // packet and pass it to the "mousevec" vector of the TOS so all TOS hooks etc. can work.
    int8_t *packet = DRIVER_DATA->ps2packet;
    packet[DRIVER_DATA->state++] = (int8_t)byte;
    if (DRIVER_DATA->state >= SM_RECEIVED)
    {
        DRIVER_DATA->state = SM_IDLE;
        volatile uint16_t * const vicky_ps2 = (uint16_t*)VICKY_MOUSE_PACKET;
        vicky_ps2[0] = (uint16_t)packet[0];
        vicky_ps2[1] = (uint16_t)packet[1];
        vicky_ps2[2] = (uint16_t)packet[2];
        
        // Emulate a IKBD mouse packet
        packet[0] = 0xf8 | (packet[0] & 3);
        api->os_callbacks.on_mouse(packet);

        //a2560u_debugnl("%02x %02x %02x %d,%d", packet[0], packet[1], packet[2], GCURX, GCURY);
    }
#else
    // This is maximum speed setup. The IKBD handling stuff (mousevec) is bypassed, we directly update line-A variables
    // and call vectors.
    // So if apps hook into the XBIOS (mousevec) they won't receive anything. But if they hook at the line-A / VDI level
    // they're ok.
    volatile uint16_t * const packet = (uint16_t*)VICKY_MOUSE_PACKET;
    packet[DRIVER_DATA->state++] = (uint16_t)byte;
    if (DRIVER_DATA->state >= SM_RECEIVED)
    {
        DRIVER_DATA->state = SM_IDLE;

        uint16_t new_cur_ms_stat;
        uint16_t x,y;
        uint16_t buttons;

        x = R16(VICKY_MOUSE_X);
        y = R16(VICKY_MOUSE_Y);
        buttons = packet[0] & 3;
    
        /* Update cur_ms_stat flags:
         * 0x01 Left mouse button status  (0=up)
         * 0x02 Right mouse button status (0=up)
         * 0x04 Reserved
         * 0x08 Reserved
         * 0x10 Reserved
         * 0x20 Mouse move flag (1=moved)
         * 0x40 Right mouse button status flag (0=hasn't changed)
         *  0x80 Left mouse button status flag  (0=hasn't changed) */         
        if (GCURX != x || GCURY != y)
            new_cur_ms_stat = 0x20; /* Mouse moved flag */
        else
            new_cur_ms_stat = 0;

        if ((buttons & 2) != (cur_ms_stat & 2))
            new_cur_ms_stat |= 0x40; /* Right button changed */

        if ((buttons & 1) != (cur_ms_stat & 1))
            new_cur_ms_stat |= 0x80; /* Left button changed */

        new_cur_ms_stat |= buttons;

        /* Update Line-A variables */
        GCURX = R16(VICKY_MOUSE_X);
        /* Note: for VICKY, mouse (0,0) top is top left of the screen, while for VDI it's bottom left */
        GCURY = linea_max_y - R16(VICKY_MOUSE_Y);
        MOUSE_BT = (MOUSE_BT & 0xfffc) | buttons;
        cur_ms_stat = new_cur_ms_stat;
        
        /* Fire callbacks */
        if (cur_ms_stat & (0x40|0x80))
            user_but();
        if (cur_ms_stat & 0x20)
            user_mot();
        /* user_cur is not supported since VICKY is in charge of drawing */
   }
#endif
}

#endif // MACHINE_A2560U