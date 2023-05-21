// Foenix Mouse driver

#include <stdint.h>

#include "regutils.h"

#include "vicky_mouse.h"
#include "vicky2.h"

 /* States the mouse PS/2 reception state machine can be in */
#define SM_IDLE 0
#define SM_BYTE0_RECEIVED 1
#define SM_BYTE1_RECEIVED 2
#define SM_BYTE2_RECEIVED 3
#define SM_RECEIVED 3


// Local state
struct _vicky_mouse_state_t
{
    vicky_mouse_event_t event;
    uint16_t state;  /* State of the state machine (really counts bytes in the packet) */
    void     (*on_change)(const vicky_mouse_event_t *);
} mouse;


void vicky_mouse_init(void (*on_change)(const vicky_mouse_event_t *), void *user_data)
{
    /* Reset mouse */
    R16(VICKY_MOUSE_CTRL) = 0;
    mouse.state = SM_IDLE;
    mouse.on_change = on_change;
    mouse.event.user_data = user_data;
}


/* Get mouse coordinates */
void vicky_mouse_state(vicky_mouse_event_t *ret)
{
    ret->x = R16(VICKY_MOUSE_X);
    ret->y = R16(VICKY_MOUSE_X);
}


void vicky_mouse_show(void)
{
    R16(VICKY_MOUSE_CTRL) |= VICKY_MOUSE_ENABLE;
}


void vicky_mouse_hide(void)
{
    R16(VICKY_MOUSE_CTRL) &= ~VICKY_MOUSE_ENABLE;
}


void vicky_mouse_ps2(int8_t byte)
{
    // Process an incoming PS2 byte and fire the on_change callback

    if (mouse.state == SM_IDLE && (byte & 0x08) != 0x08)
        return; /* We're lost. Ignore the packet. This is not even safe because if the packet contains a 08 we're fooled. */

    // The VICKY doesn't allow to set mouse coordinates. In case the user wants to move the mouse
    // to a particular location, we can't do that while we're in the middle of receiving a packet.    
    // We cache packet data so we always know where we're at. In case we're requested to move the
    // mouse to a particular location, we can then (dummy-)complete the current packet.
    int8_t *packet = mouse.event.ps2packet;
    packet[mouse.state++] = (int8_t)byte;
    
    if (mouse.state >= SM_RECEIVED)
    {
        mouse.state = SM_IDLE;
        
        volatile uint16_t * const vicky_ps2 = (uint16_t*)VICKY_MOUSE_PACKET;
        vicky_ps2[0] = (uint16_t)packet[0];
        vicky_ps2[1] = (uint16_t)packet[1];
        vicky_ps2[2] = (uint16_t)packet[2];
        
        // ... Vicky automatically updates mouse x and y ...

        // Detect what's changed
        vicky_mouse_event_t *ev = &mouse.event;
        mouse.event.changed = 0;
        if (ev->x == R16(VICKY_MOUSE_X))
            ev->changed = 1;
        if (ev->y == R16(VICKY_MOUSE_Y))
            ev->changed = 2;
        if ((ev->buttons & 1) == (packet[0] & 1))
            ev->changed = 4;
        if ((ev->buttons & 2) == (packet[0] & 2))
            ev->changed = 8;

        // Update state
        ev->x = R16(VICKY_MOUSE_X);
        ev->y = R16(VICKY_MOUSE_X);
        ev->buttons = packet[0] & 3;

        if (mouse.on_change)
            mouse.on_change(ev);
    }
}
