#ifndef VICKY_MOUSE_H
#define VICKY_MOUSE_H

typedef struct
{
    uint16_t x;
    uint16_t y;
    uint16_t buttons;
    uint16_t changed;      // 1=X, 2=Y, 4=BUTTON1, 8=BUTTON2
    int8_t   ps2packet[3]; // This is only here to facilitate the creation on a IKBD packet for EmuTOS. Shoud otherwize be hidden in _vicky_mouse_state_t
    void     *user_data;
} vicky_mouse_event_t;


void vicky_mouse_init(void (*on_change)(const vicky_mouse_event_t *), void *user_data);
void vicky_mouse_state(vicky_mouse_event_t *ret);
void vicky_mouse_show(void);
void vicky_mouse_hide(void);
// Mouse handling in the Foenix is peculiar. The only way to move the mouse is to send PS/2 packet into VICKY
void vicky_mouse_ps2(int8_t byte);

#endif
