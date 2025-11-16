/* Function numbers for the dispatched of the A2560 Foenix lib
 * Author: Vincent Barrilliot
 */

#ifndef _FNX_TRAP_FN_NUMBERS_H
#define _FNX_TRAP_FN_NUMBERS_H

#define FNX_ROOT_DISPATCHER 0 /* (int32_t(*)(...)) x(void) */
#define FNX_DISPATCH_TABLE  1 /* void     x(void)  */
#define FNX_FUNCTIONS_COUNT 2 /* uint16_t x(void)  */

#define FNX_SYSTEM_INFO     12 /* a2560_system_info */

/* Interrupts */
#define FNX_IRQ_FN_BASE     30
#define FNX_IRQ_INIT        (FNX_IRQ_FN_BASE+00)
#define FNX_IRQ_MASK_ALL    (FNX_IRQ_FN_BASE+01)
#define FNX_IRQ_RESTORE     (FNX_IRQ_FN_BASE+02)
#define FNX_IRQ_ENABLE      (FNX_IRQ_FN_BASE+03)
#define FNX_IRQ_DISABLE     (FNX_IRQ_FN_BASE+04)
#define FNX_IRQ_ACKNOWLEDGE (FNX_IRQ_FN_BASE+05)
#define FNX_IRQ_SET_HANDLER (FNX_IRQ_FN_BASE+06)

/*  Timers */
#define FNX_TIMER_BASE      40
#define FNX_TIMER_INIT      (FNX_TIMER_BASE+00)
#define FNX_TIMER_SET       (FNX_TIMER_BASE+01)
#define FNX_TIMER_ENABLE    (FNX_TIMER_BASE+02)
#define FNX_TIMER_RUN_CALIBRATION (FNX_TIMER_BASE+03)

/* Super IO */
#define FNX_SUPERIO_BASE    50
#define FNX_SUPERIO_INIT    (FNX_SUPERIO_BASE+00)

/* SN76489 Programmable Sound Generator */
#define FNX_SN76489_BASE        80
#define FNX_SN76489_SELECT      (FNX_SN76489_BASE+0)
#define FNX_SN76489_MUTE_ALL    (FNX_SN76489_BASE+1)
#define FNX_SN76489_FREQ        (FNX_SN76489_BASE+2)
#define FNX_SN76489_TONE        (FNX_SN76489_BASE+3)
#define FNX_SN76489_ATTENUATION (FNX_SN76489_BASE+4)
#define FNX_SN76489_NOISE_SOURCE    (FNX_SN76489_BASE+5)

/* WM8776 mixer/codec */
#define FNX_WM8776_BASE         100
#define FNX_WM8776_INIT         (FNX_WM8776_BASE+0)
#define FNX_WM8776_DEINIT       (FNX_WM8776_BASE+1)
#define FNX_WM8776_SEND         (FNX_WM8776_BASE+2)
#define FNX_WM8776_SET_DIGITAL_VOLUME   (FNX_WM8776_BASE+3)
#define FNX_WM8776_GET_DIGITAL_VOLUME   (FNX_WM8776_BASE+4)

/* Keyboard */
#define FNX_KBD_BASE                    150
#define FNX_KBD_INIT                    (FNX_KBD_BASE+0)
#define FNX_PS2_SET_KEY_UP_HANDLER      (FNX_KBD_BASE+1)
#define FNX_PS2_SET_KEY_DOWN_HANDLER    (FNX_KBD_BASE+2)
#define FNX_PS2_SET_MOUSE_HANDLER       (FNX_KBD_BASE+3)

/* Real Time Clock */
#define FNX_RTC_BASE                170
#define FNX_RTC_INIT                (FNX_RTC_BASE+0)
#define FNX_RTC_SET_TICK_RATE       (FNX_RTC_BASE+1)
#define FNX_RTC_ENABLE_TICKS        (FNX_RTC_BASE+2)
#define FNX_RTC_SET_TICK_HANDLER    (FNX_RTC_BASE+3)
#define FNX_RTC_GET_TICKS           (FNX_RTC_BASE+4)
#define FNX_RTC_SET_DATETIME        (FNX_RTC_BASE+5)
#define FNX_RTC_GET_DATETIME        (FNX_RTC_BASE+6)

#endif
