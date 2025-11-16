/* Bindings for the Foenix Library. This part can be used in client applications.
 * Author: Vincent Barrilliot
 */

#ifndef _FNX_TRAP_BINDINGS_H_
#define _FNX_TRAP_BINDINGS_H_

#include <stdint.h>
#include <stdbool.h>
#include "a2560_struct.h"

#define TRAP_NUMBER 15

/* We use the stack for maximum compatibility, we don't pass data in registers */
#define ARGS_ON_STACK

void ARGS_ON_STACK fnx_system_info(struct foenix_system_info_t *ret);

/* Interrupts */
void ARGS_ON_STACK fnx_irq_init(void);
void ARGS_ON_STACK fnx_irq_mask_all(uint16_t *save);
void ARGS_ON_STACK fnx_irq_restore(const uint16_t *save);
void ARGS_ON_STACK fnx_irq_enable(uint16_t irq_id);
void ARGS_ON_STACK fnx_irq_disable(uint16_t irq_id);
void ARGS_ON_STACK fnx_irq_acknowledge(uint8_t irq_id);
void ARGS_ON_STACK *fnx_irq_set_handler(uint16_t irq_id, void *handler);

/* Timers */
void ARGS_ON_STACK fnx_timer_init(void);
void ARGS_ON_STACK fnx_timer_set(uint16_t timer, uint32_t frequency, bool repeat, void *handler);
void ARGS_ON_STACK fnx_timer_enable(uint16_t timer, bool enable);
uint32_t ARGS_ON_STACK fnx_timer_run_calibration(uint32_t );

/* SuperIO */
void ARGS_ON_STACK fnx_superio_init(void);

/* SN76489 programmable sound generator */
void ARGS_ON_STACK fnx_sn76489_select(int number);
void ARGS_ON_STACK fnx_sn76489_mute_all(void);
void ARGS_ON_STACK fnx_sn76489_freq(uint8_t voice, uint16_t frequency);
void ARGS_ON_STACK fnx_sn76489_tone(uint8_t voice, uint16_t period);
void ARGS_ON_STACK fnx_sn76489_attenuation(uint8_t voice, uint8_t attenuation);
/* Set the noise source. type: 0:periodid, 1:white ; source: 0:N/512 1:N/1024 2:N/2048 3:Tone generator 3 */
void ARGS_ON_STACK fnx_sn76489_noise_source(uint8_t type, uint8_t source);

/* Keyboard */
void ARGS_ON_STACK fnx_kbd_init(const uint32_t *counter, uint16_t counter_freq);
/* PS/2 stuff. Note: we don't assume the keyboard is PS/2 because e.g the K has a keyboard with a controller (Maurice) which is not PS/2*/
scancode_handler_t ARGS_ON_STACK fnx_ps2_set_key_up_handler(scancode_handler_t);
scancode_handler_t ARGS_ON_STACK fnx_ps2_set_key_down_handler(scancode_handler_t);
mouse_packet_handler_t ARGS_ON_STACK fnx_ps2_set_mouse_handler(mouse_packet_handler_t);

/* Real Time Clock */
void fnx_bq4802ly_init(void);
void fnx_bq4802ly_set_tick_rate(uint16_t rate);
void fnx_bq4802ly_enable_ticks(bool enable);
tick_handler_t fnx_bq4802ly_get_tick_handler(void);
/* The handler must save all the registers, it uses and terminate with rte */
void fnx_bq4802ly_set_tick_handler(tick_handler_t handler);
uint32_t fnx_bq4802ly_get_ticks(void);
void fnx_bq4802ly_set_datetime(uint8_t day, uint8_t month, uint16_t year, uint8_t hour, uint8_t minute, uint8_t second);
void fnx_bq4802ly_get_datetime(uint8_t *day, uint8_t *month, uint16_t *year, uint8_t *hour, uint8_t *minute, uint8_t *second);

#endif
