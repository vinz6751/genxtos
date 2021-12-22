/* PS/2 mouse driver 
 * 
 * Authors:
 *	Vincent Barrilliot
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 *
 * This is a mouse driver.
 */

 #include <stdint.h>
 #include <stddef.h>
 #include "ps2.h"

 /* Prototypes */
static const char driver_name[] = "PS/2 Mouse";
static bool init(struct ps2_driver_api_t *api);
static bool can_drive(const uint8_t ps2_device_type[]);
static void process(const struct ps2_driver_api_t *api, uint8_t byte);

/* Driver itself */
const struct ps2_driver_t ps2_mouse_driver =
{
    .name = driver_name,
    .init = init,
    .can_drive = can_drive,
    .process = process
};

 /* States the mouse state machine can be in */
typedef enum sm_state
{
    SM_IDLE = 0,
    SM_STATUS,
    SM_X
} sm_state_t;

 /* Internal state of the driver */
static struct ps2_mouse_local_t
{
    sm_state_t state;  /* State of the state machine */
    uint16_t x;
    uint16_t y;
} L;

/* Convenience */
#define DRIVER_DATA ((struct ps2_mouse_local_t*)api->driver_data)

static bool init(struct ps2_driver_api_t *api)
{
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

static void process(const struct ps2_driver_api_t *api, uint8_t byte)
{
    /* Factoring helpers, see the meat below */
    #define STATE ((struct ps2_mouse_local_t*)(api->driver_data))->state
}