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
#include "interrupts.h"
#include "keyboard.h"
#include "sn76489.h"
#include "superio.h"
#include "timer.h"
#include "wm8776.h"


int32_t trap_dispatch(uint16_t *args_on_stack)
{
	uint16_t *args = (uint16_t *)args_on_stack;

#if 1 /* Log what's on the stack to help debugging */
	uint8_t *s = (uint8_t*)args_on_stack;
	for (int i=-16;i<16;i++)
		a2560_debug("%02x ", s[i]);
	a2560_debugnl("");
#endif

	uint16_t function_number = *args++;

	a2560_debugnl("args_on_stack: %p, function:%d arguments: %p ", args_on_stack, function_number, args);

	switch (function_number) {
	case FNX_SYSTEM_INFO: {
		struct foenix_system_info_t *arg1 = *((struct foenix_system_info_t **)args);
		a2560_debugnl("a2560_system_info(%p)", arg1);
		a2560_system_info(arg1);
		return 0;
	}
	/* Interrupts */
	case FNX_IRQ_INIT: a2560_irq_init(); break;
	case FNX_IRQ_MASK_ALL: a2560_irq_mask_all(*((uint16_t**)args)); break;
	case FNX_IRQ_RESTORE: a2560_irq_restore(*((uint16_t**)args)); break;
	case FNX_IRQ_ENABLE: a2560_irq_enable(*((uint16_t*)args)); break;
	case FNX_IRQ_DISABLE: a2560_irq_disable(*((uint16_t*)args)); break;
	case FNX_IRQ_ACKNOWLEDGE: a2560_irq_disable(*((uint16_t*)args)); break;; /* it's really uint8_t */
	case FNX_IRQ_SET_HANDLER: a2560_irq_set_handler(*((uint16_t*)args++), *((void**)args)); break;

	/* Timers */
	case FNX_TIMER_INIT: a2560_timer_init(); break;
	case FNX_TIMER_SET: a2560_set_timer(*((uint16_t*)args++), *((uint32_t*)args++), *((uint16_t*)args++), *((void**)args)); break;
	case FNX_TIMER_ENABLE: a2560_timer_enable(*((uint16_t*)args++), *((uint16_t*)args)); break;
	case FNX_TIMER_RUN_CALIBRATION: a2560_run_calibration(*((uint32_t*)args)); break;

	/* SuperIO */
	case FNX_SUPERIO_INIT: superio_init(); break;

	/* SN76489 programmable sound generator */
	case FNX_SN76489_SELECT: sn76489_select(*((uint16_t*)args)); break;
	case FNX_SN76489_MUTE_ALL: sn76489_mute_all(); break;
	case FNX_SN76489_FREQ: sn76489_freq(*((uint16_t*)args++), *((uint32_t*)args)); break;
	case FNX_SN76489_TONE: sn76489_tone(*((uint16_t*)args++), *((uint32_t*)args)); break;
	case FNX_SN76489_ATTENUATION: sn76489_attenuation(*((uint16_t*)args++), *((uint16_t*)args)); break;
	case FNX_SN76489_NOISE_SOURCE: sn76489_noise_source(*((uint16_t*)args++), *((uint16_t*)args)); break;

	/* WM8776 mixer / codec */
	case FNX_WM8776_INIT: wm8776_init(); break;
	case FNX_WM8776_DEINIT: wm8776_deinit(); break;
	case FNX_WM8776_SEND: wm8776_send(*((uint16_t*)args)); break;
	case FNX_WM8776_SET_DIGITAL_VOLUME: wm8776_set_digital_volume(*((uint16_t*)args)); break;
	case FNX_WM8776_GET_DIGITAL_VOLUME: return wm8776_get_digital_volume();

	/* PS/2 keyboard and mouse */
	case FNX_KBD_INIT: a2560_kbd_init(*((uint32_t**)args++), *((uint16_t*)args)); break;
	case FNX_PS2_SET_KEY_UP_HANDLER: return (int32_t)a2560_ps2_set_key_up_handler(*((void(**)(uint8_t))args));
	case FNX_PS2_SET_KEY_DOWN_HANDLER: return (int32_t)a2560_ps2_set_key_down_handler(*((void(**)(uint8_t))args));
	case FNX_PS2_SET_MOUSE_HANDLER: return (int32_t)a2560_ps2_set_mouse_handler(*((void(**)(int8_t*))args));
	
	#if 0
	case FNX_PS2_SET_KEY_UP_HANDLER: return (int32_t)a2560_ps2_set_key_up_handler(*((void(**)(uint8_t))args));
	case FNX_PS2_SET_KEY_DOWN_HANDLER: return (int32_t)a2560_ps2_set_key_down_handler(*((void(**)(uint8_t))args));
	case FNX_PS2_SET_MOUSE_HANDLER: return (int32_t)a2560_ps2_set_mouse_handler(*((void(**)(int8_t*))args));
	#endif

	default:
		return -1;
	}

	return 0;
}
