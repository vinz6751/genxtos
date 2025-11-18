/* Dispatcher for the trap interface, leverages the "dispatch.h" definitions and is called from the trap.c file
 * It's called "root" because I think it could be extended to support multiple dispatch tables for different libraries.
 * Author: Vincent Barrilliot
 */

#include <stdint.h>
#include "config.h"
#include "a2560_debug.h"
#include "trap_fn_numbers.h"

/* Modules containing dispatched functions */
#include "a2560.h"
#include "a2560_struct.h"
#include "bq4802ly.h"
#include "interrupts.h"
#include "keyboard.h"
#include "sn76489.h"
#include "superio.h"
#include "timer.h"
#include "wm8776.h"


int32_t trap_dispatch(uint16_t *args_on_stack)
{
	uint16_t *args = (uint16_t *)args_on_stack;
	uint16_t function_number = *args++;

	a2560_debugnl("args_on_stack: %p, function:%d arguments: %p ", args_on_stack, function_number, args);
#if 0 /* Log what's on the stack to help debugging */
	uint8_t *s = (uint8_t*)args_on_stack;
	for (int i=0;i<16;i++)
		a2560_debug("%02x ", s[i]);
	a2560_debugnl("");
#endif

	switch (function_number) {
	/* System information */
	case FNX_SYSTEM_INFO: a2560_system_info(*((struct foenix_system_info_t **)args)); break;

	/* Interrupts */
	case FNX_IRQ_INIT: a2560_irq_init(); break;
	case FNX_IRQ_MASK_ALL: a2560_irq_mask_all(*((uint16_t**)args)); break;
	case FNX_IRQ_RESTORE: a2560_irq_restore(*((uint16_t**)args)); break;
	case FNX_IRQ_ENABLE: a2560_irq_enable(*((uint16_t*)args)); break;
	case FNX_IRQ_DISABLE: a2560_irq_disable(*((uint16_t*)args)); break;
	case FNX_IRQ_ACKNOWLEDGE: a2560_irq_disable(*((uint16_t*)args)); break;; /* it's really uint8_t */
	case FNX_IRQ_SET_HANDLER: { struct p_t { uint16_t a; void *b; } *p = (struct p_t*)args; a2560_irq_set_handler(p->a, p->b); break; }

	/* Timers */
	case FNX_TIMER_INIT: a2560_timer_init(); break;
	case FNX_TIMER_SET: { struct p_t { uint16_t a; uint32_t b; bool c; void *d; } *p = (struct p_t*)args; a2560_set_timer(p->a, p->b, p->c, p->d); break; }
	case FNX_TIMER_ENABLE: { struct p_t { uint16_t a; bool b; } *p = (struct p_t*)args; a2560_timer_enable(p->a, p->b); break; }
	case FNX_TIMER_RUN_CALIBRATION: a2560_run_calibration(*((uint32_t*)args)); break;

	/* SuperIO */
	case FNX_SUPERIO_INIT: superio_init(); break;

	/* SN76489 programmable sound generator */
	case FNX_SN76489_SELECT: sn76489_select(*((uint8_t*)args)); break;
	case FNX_SN76489_MUTE_ALL: sn76489_mute_all(); break;
	case FNX_SN76489_FREQ: { struct p_t { uint8_t a; uint16_t b; } *p = (struct p_t*)args; sn76489_freq(p->a, p->b); break; }
	case FNX_SN76489_TONE: { struct p_t { uint8_t a; uint16_t b; } *p = (struct p_t*)args; sn76489_tone(p->a, p->b); break; }
	case FNX_SN76489_ATTENUATION: { struct p_t { uint8_t a; uint8_t b; } *p = (struct p_t*)args; sn76489_attenuation(p->a, p->b); break; }
	case FNX_SN76489_NOISE_SOURCE: { struct p_t { uint8_t a; uint8_t b; } *p = (struct p_t*)args; sn76489_noise_source(p->a, p->b); break; }

	/* WM8776 mixer / codec */
	case FNX_WM8776_INIT: wm8776_init(); break;
	case FNX_WM8776_DEINIT: wm8776_deinit(); break;
	case FNX_WM8776_SEND: wm8776_send(*((uint16_t*)args)); break;
	case FNX_WM8776_SET_DIGITAL_VOLUME: wm8776_set_digital_volume(*((uint16_t*)args)); break;
	case FNX_WM8776_GET_DIGITAL_VOLUME: return wm8776_get_digital_volume();

	/* PS/2 keyboard and mouse */
	case FNX_KBD_INIT:  { struct p_t { const uint32_t *a; uint16_t b; } *p = (struct p_t*)args; a2560_kbd_init(p->a, p->b); break; }
	case FNX_PS2_SET_KEY_UP_HANDLER: return (int32_t)a2560_ps2_set_key_up_handler(*((scancode_handler_t*)args));
	case FNX_PS2_SET_KEY_DOWN_HANDLER: return (int32_t)a2560_ps2_set_key_down_handler(*((scancode_handler_t*)args));
	case FNX_PS2_SET_MOUSE_HANDLER: return (int32_t)a2560_ps2_set_mouse_handler(*((mouse_packet_handler_t*)args));

	/* Real time Clock */
	case FNX_RTC_INIT: bq4802ly_init(); break;
	case FNX_RTC_SET_TICK_RATE: bq4802ly_set_tick_rate(*((uint16_t*)args)); break;
	case FNX_RTC_ENABLE_TICKS: bq4802ly_enable_ticks(*((uint16_t*)args)); break;
	case FNX_RTC_SET_TICK_HANDLER: return (int32_t)bq4802ly_set_tick_handler(*((void(**)(void))args));
	case FNX_RTC_GET_TICKS: return (int32_t)bq4802ly_get_ticks();

	// FIXME !
	case FNX_RTC_SET_DATETIME: bq4802ly_set_datetime(*((uint16_t*)args++), *((uint16_t*)args++), *((uint16_t*)args++), *((uint16_t*)args++), *((uint16_t*)args++), *((uint16_t*)args++)); break;
	case FNX_RTC_GET_DATETIME: bq4802ly_get_datetime(*((uint16_t**)args++), *((uint16_t**)args++), *((uint16_t**)args++), *((uint16_t**)args++), *((uint16_t**)args++), *((uint16_t**)args++)); break;

	default:
		return -1;
	}

	return 0;
}
