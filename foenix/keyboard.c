/* Setups keyboard/mouse inputs sources. This is not handling key mappings. */

#include <stdint.h>
#include "foenix.h"
#include "a2560_debug.h"
#include "cpu.h"
#include "interrupts.h"
#include "keyboard.h"
#include "ps2.h"
#include "ps2_keyboard.h"
#include "ps2_mouse_a2560.h"

/* The IRQ handlers */
void a2560_irq_ps2kbd(void);
void a2560_irq_ps2mouse(void);

static void do_nothing(uint8_t dummy) {
}


/* List here all drivers we support */
static const struct ps2_driver_t * const drivers[] = {
    &ps2_keyboard_driver,
    &ps2_mouse_driver_a2560u
};

/* The following must be provided by the called (we use it for timeouts)
 * counter: a ever increasing counter
 * counter_freq = its frequency in Hz;
 */
void a2560_kbd_init(const uint32_t *counter, uint16_t counter_freq)
{
    /* Setup the PS/2 stuff */
    /* TODO: on the A2560K, setup MAURICE too/instead ? */

    a2560_debugnl("a2560_kbd_init");

    /* Disable IRQ while we're configuring */
    a2560_irq_disable(INT_KBD_PS2);
    a2560_irq_disable(INT_MOUSE);

    /* Explain our setup to the PS/2 subsystem */
    ps2_config.counter      = counter;
    ps2_config.counter_freq = counter_freq;
    ps2_config.port_data    = (uint8_t*)PS2_DATA;
    ps2_config.port_status  = (uint8_t*)PS2_CMD;
    ps2_config.port_cmd     = (uint8_t*)PS2_CMD;
    ps2_config.n_drivers    = sizeof(drivers)/sizeof(struct ps2_driver_t*);
    ps2_config.drivers      = drivers;

    ps2_config.callbacks.on_key_up = ps2_config.callbacks.on_key_down = (scancode_handler_t)do_nothing;
    ps2_config.callbacks.on_mouse = (mouse_packet_handler_t)do_nothing;

#if 0
    a2560_debugnl("port_data: %p", ps2_config.port_data);
    a2560_debugnl("port_status: %p", ps2_config.port_status);
    a2560_debugnl("port_cmd: %p", ps2_config.port_cmd);
#endif

    ps2_init();

    /* Register GAVIN interrupt handlers */
    cpu_set_vector(INT_PS2KBD_VECN, (uint32_t)a2560_irq_ps2kbd);
    cpu_set_vector(INT_PS2MOUSE_VECN, (uint32_t)a2560_irq_ps2mouse);

    /* Acknowledge any pending interrupt */
    a2560_irq_acknowledge(INT_KBD_PS2);
    a2560_irq_acknowledge(INT_MOUSE);

    /* Go ! */
    a2560_debugnl("Enabling GAVIN PS2/mouse irqs");
    a2560_irq_enable(INT_KBD_PS2);
    a2560_irq_enable(INT_MOUSE);
}


scancode_handler_t a2560_ps2_set_key_up_handler(void (*handler)(uint8_t scancode)) {
    scancode_handler_t previous = ps2_config.callbacks.on_key_up;
    ps2_config.callbacks.on_key_up = handler;
    return previous;
}


scancode_handler_t a2560_ps2_set_key_down_handler(void (*handler)(uint8_t scancode)) {
    scancode_handler_t previous = ps2_config.callbacks.on_key_down;
    ps2_config.callbacks.on_key_down = handler;
    return previous;
}


mouse_packet_handler_t a2560_ps2_set_mouse_handler(void (*handler)(int8_t *packet)) {
    /* Is it safe or should we have a semaphore so we don't change the handler mid-reception of a mouse packet ?*/
    mouse_packet_handler_t previous = ps2_config.callbacks.on_mouse;
    ps2_config.callbacks.on_mouse = handler;
    return previous;
}
