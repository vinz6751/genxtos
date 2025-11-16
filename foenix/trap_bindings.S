 #include "trap_fn_numbers.h"

#define CONCAT1(a, b) CONCAT2(a, b)
#define CONCAT2(a, b) a ## b
#define SYM(x) CONCAT1 (_,x)  /* This exists to cater for some compilers prefixing symbols with _ and others not */

#define TRAP_NUMBER 15

    .GLOBAL SYM(fnx_system_info)
SYM(fnx_system_info):
    move.l 	4(sp),-(sp)
    move.w	#FNX_SYSTEM_INFO,-(sp)
    trap	#TRAP_NUMBER
    addq.l	#6,sp
    rts

/* Interrupts ****************************************************************/

    .GLOBAL SYM(fnx_irq_init)
SYM(fnx_irq_init):
    move.w  #FNX_IRQ_INIT,-(sp)
    trap    #TRAP_NUMBER
    addq.w  #2,sp
    rts

    .GLOBAL SYM(fnx_mask_all)
SYM(fnx_irq_mask_all):
    move.w  4(sp),-(sp)
    move.w  #FNX_IRQ_MASK_ALL,-(sp)
    trap    #TRAP_NUMBER
    addq.l  #4,sp
    rts

    .GLOBAL SYM(fnx_irq_restore)
SYM(fnx_unmask_all):
    move.w  4(sp),-(sp)
    move.w  #FNX_IRQ_RESTORE,-(sp)
    trap    #TRAP_NUMBER
    addq.l  #4,sp
    rts

    .GLOBAL SYM(fnx_irq_enable)
SYM(fnx_irq_enable):
    move.w  4(sp),-(sp)
    move.w  #FNX_IRQ_ENABLE,-(sp)
    trap    #TRAP_NUMBER
    addq.l  #4,sp
    rts

    .GLOBAL SYM(fnx_irq_disable)
SYM(fnx_irq_disable):
    move.w  4(sp),-(sp)
    move.w  #FNX_IRQ_DISABLE,-(sp)
    trap    #TRAP_NUMBER
    addq.l  #4,sp
    rts

    .GLOBAL SYM(fnx_irq_acknowledge)
SYM(fnx_irq_acknowledge):
    move.w  4(sp),-(sp)
    move.w  #FNX_IRQ_ACKNOWLEDGE,-(sp)
    trap    #TRAP_NUMBER
    addq.l  #4,sp
    rts

    .GLOBAL SYM(fnx_irq_set_handler)
SYM(fnx_irq_set_handler):
    move.l  4(sp),-(sp)
    move.w  #FNX_IRQ_SET_HANDLER,-(sp)
    trap    #TRAP_NUMBER
    addq.l  #6,sp
    rts


/* Timers ********************************************************************/
    .GLOBAL SYM(fnx_timer_init)
SYM(fnx_timer_init):
    move.w  #FNX_TIMER_INIT,-(sp)
    trap    #TRAP_NUMBER
    addq.l  #2,sp
    rts

    .GLOBAL SYM(fnx_timer_set)
SYM(fnx_timer_set):
    lea     4(sp),a0
    move.l  8(a0),-(sp)
    move.w  6(a0),-(sp)
    move.l  2(a0),-(sp)
    move.w  0(a0),-(sp)
    move.w  FNX_TIMER_SET,-(sp)
    trap    #TRAP_NUMBER
    lea     14(sp),sp
    rts

    .GLOBAL SYM(fnx_timer_enable)
fnx_timer_enable:
    move.w  4(sp),-(sp)
    move.w  FNX_TIMER_ENABLE,-(sp)
    trap    #TRAP_NUMBER
    addq.l  #4,sp
    rts

    .GLOBAL SYM(fnx_timer_run_calibration)
SYM(fnx_timer_run_calibration):
    move.l  4(sp),-(sp)
    move.w  FNX_TIMER_RUN_CALIBRATION,-(sp)
    trap    #TRAP_NUMBER
    addq.l  #6,sp
    rts

/* Super IO ******************************************************************/
    .GLOBAL SYM(fnx_superio_init)
SYM(fnx_superio_init):
    move.w  FNX_SUPERIO_INIT,-(sp)
    trap    #TRAP_NUMBER
    addq.l  #2,sp
    rts

/* SN76489 Programmable Sound Generator **************************************/
    .GLOBAL SYM(fnx_sn76489_select)
SYM(fnx_sn76489_select):
    move.w  4(sp),-(sp)
    move.w  #FNX_SN76489_SELECT,-(sp)
    trap    #TRAP_NUMBER
    addq.l  #4,sp
    rts

    .GLOBAL SYM(fnx_sn76489_mute_all)
SYM(fnx_sn76489_mute_all):
    move.w  #FNX_SN76489_MUTE_ALL,-(sp)
    trap    #TRAP_NUMBER
    addq.l  #2,sp
    rts

    .GLOBAL SYM(fnx_sn76489_freq)
SYM(fnx_sn76489_freq):
    lea     4(sp),a0
    move.w  2(a0),-(sp)
    move.w  0(a0),-(sp)
    move.w  #FNX_SN76489_FREQ,-(sp)
    trap    #TRAP_NUMBER
    addq.l  #6,sp
    rts

    .GLOBAL SYM(fnx_sn76489_tone)
SYM(fnx_sn76489_tone):
    lea     4(sp),a0
    move.w  2(a0),-(sp)
    move.w  0(a0),-(sp)
    move.w  #FNX_SN76489_TONE,-(sp)
    trap    #TRAP_NUMBER
    addq.l  #6,sp
    rts

    .GLOBAL SYM(fnx_sn76489_attenuation)
SYM(fnx_sn76489_attenuation):
    lea     4(sp),a0
    move.w  2(a0),-(sp)
    move.w  0(a0),-(sp)
    move.w  #FNX_SN76489_ATTENUATION,-(sp)
    trap    #TRAP_NUMBER
    addq.l  #6,sp
    rts

    .GLOBAL SYM(fnx_sn76489_noise_source)
SYM(fnx_sn76489_noise_source):
    lea     4(sp),a0
    move.w  2(a0),-(sp)
    move.w  0(a0),-(sp)
    move.w  #FNX_SN76489_NOISE_SOURCE,-(sp)
    trap    #TRAP_NUMBER
    addq.l  #6,sp
    rts

/* WM8776 mixer and codec ****************************************************/
    .GLOBAL SYM(fnx_wm8776_init)
SYM(fnx_wm8776_init):
    move.w  #FNX_WM8776_INIT,-(sp)
    trap    #TRAP_NUMBER
    addq.l  #2,sp
    rts

    .GLOBAL SYM(fnx_wm8776_init)
SYM(fnx_wm8776_deinit):
    move.w  #FNX_WM8776_DEINIT,-(sp)
    trap    #TRAP_NUMBER
    addq.l  #2,sp
    rts

    .GLOBAL SYM(fnx_wm8776_send)
SYM(fnx_wm8776_send):
    move.w  4(sp),-(sp)
    move.w  #FNX_WM8776_DEINIT,-(sp)
    trap    #TRAP_NUMBER
    addq.l  #4,sp
    rts

    .GLOBAL SYM(fnx_wm8776_set_digital_volume)
SYM(fnx_wm8776_set_digital_volume):
    move.w  4(sp),-(sp)
    move.w  #FNX_WM8776_SET_DIGITAL_VOLUME,-(sp)
    trap    #TRAP_NUMBER
    addq.l  #4,sp
    rts

    .GLOBAL SYM(fnx_wm8776_get_digital_volume)
SYM(fnx_wm8776_get_digital_volume):
    move.w  #FNX_WM8776_GET_DIGITAL_VOLUME,-(sp)
    trap    #TRAP_NUMBER
    addq.l  #2,sp
    rts

/* Keyboard and mouse ********************************************************/
    .GLOBAL SYM(fnx_kbd_init)
SYM(fnx_kbd_init):
    lea     4(sp),a0
    move.w  4(a0),-(sp)
    move.l  0(a0),-(sp)
    move.w  #FNX_KBD_INIT,-(sp)
    trap    #TRAP_NUMBER
    addq.l  #8,sp
    rts

    .GLOBAL SYM(fnx_ps2_set_key_up_handler)
SYM(fnx_ps2_set_key_up_handler):
    move.l  4(sp),-(sp)
    move.w  #FNX_PS2_SET_KEY_UP_HANDLER,-(sp)
    trap    #TRAP_NUMBER
    addq.l  #6,sp
    rts

    .GLOBAL SYM(fnx_ps2_set_key_down_handler)
SYM(fnx_ps2_set_key_down_handler):
    move.l  4(sp),-(sp)
    move.w  #FNX_PS2_SET_KEY_DOWN_HANDLER,-(sp)
    trap    #TRAP_NUMBER
    addq.l  #6,sp
    rts

    .GLOBAL SYM(fnx_ps2_set_mouse_handler)
SYM(fnx_ps2_set_mouse_handler):
    move.l  4(sp),-(sp)
    move.w  #FNX_PS2_SET_MOUSE_HANDLER,-(sp)
    trap    #TRAP_NUMBER
    addq.l  #6,sp
    rts

/* Real Time Clock ***********************************************************/
    .GLOBAL SYM(fnx_bq4802ly_init)
SYM(fnx_bq4802ly_init):
    move.w  #FNX_RTC_INIT,-(sp)
    trap    #TRAP_NUMBER
    addq.l  #2,sp
    rts

    .GLOBAL SYM(fnx_bq4802ly_set_tick_rate)
SYM(fnx_bq4802ly_set_tick_rate):
    move.w  4(sp),-(sp)
    move.w  #FNX_RTC_SET_TICK_RATE,-(sp)
    trap    #TRAP_NUMBER
    addq.l  #4,sp
    rts

    .GLOBAL SYM(fnx_bq4802ly_enable_ticks)
SYM(fnx_bq4802ly_enable_ticks):
    move.w  4(sp),-(sp)
    move.w  #FNX_RTC_ENABLE_TICKS,-(sp)
    trap    #TRAP_NUMBER
    addq.l  #4,sp
    rts

    .GLOBAL SYM(fnx_bq4802ly_set_tick_handler)
SYM(fnx_bq4802ly_set_tick_handler):
    move.l  4(sp),-(sp)
    move.w  #FNX_RTC_SET_TICK_HANDLER,-(sp)
    trap    #TRAP_NUMBER
    addq.l  #6,sp
    rts

    .GLOBAL SYM(fnx_bq4802ly_get_ticks)
SYM(fnx_bq4802ly_get_ticks):
    move.w  #FNX_RTC_GET_TICKS,-(sp)
    trap    #TRAP_NUMBER
    addq.l  #2,sp
    rts

    .GLOBAL SYM(fnx_bq4802ly_set_datetime)
SYM(fnx_bq4802ly_set_datetime):
    lea     4(sp),a0
    move.w  10(a0),-(sp)
    move.w  8(a0),-(sp)
    move.w  6(a0),-(sp)
    move.w  4(a0),-(sp)
    move.w  2(a0),-(sp)
    move.w  0(a0),-(sp)
    move.w  #FNX_RTC_SET_DATETIME,-(sp)
    trap    #TRAP_NUMBER
    lea     16(sp),sp
    rts

    .GLOBAL SYM(fnx_bq4802ly_get_datetime)
SYM(fnx_bq4802ly_get_datetime):
    lea     4(sp),a0
    move.l  20(a0),-(sp)
    move.l  16(a0),-(sp)
    move.l  12(a0),-(sp)
    move.l  8(a0),-(sp)
    move.l  4(a0),-(sp)
    move.l  0(a0),-(sp)
    move.w  #FNX_RTC_SET_DATETIME,-(sp)
    trap    #TRAP_NUMBER
    lea     26(sp),sp
    rts
