/*
 * a2560u - Glue between EmuTOS and Foenix Retro Systems A2560U specific functions
 *
 * Copyright (C) 2013-2022 The EmuTOS development team
 *
 * Authors:
 *  VB   Vincent Barrilliot
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#define ENABLE_KDEBUG

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#include "emutos.h"

#ifdef MACHINE_A2560U

#include "portab.h"
#include "vectors.h"
#include "tosvars.h"
#include "bios.h"
#include "biosdefs.h"
#include "processor.h"
#include "biosext.h"            /* for cache control routines */
#include "gemerror.h"
#include "ikbd.h"               /* for call_mousevec() */
#include "screen.h"
#include "delay.h"
#include "asm.h"
#include "lineavars.h"
#include "string.h"
#include "disk.h"
#include "biosmem.h"
#include "bootparams.h"
#include "doprintf.h"
#include "machine.h"
#include "has.h"
#include "../bdos/bdosstub.h"
#include "screen.h"
#include "serport.h" // push_serial_iorec
#include "stdint.h"
#include "../foenix/foenix.h"
#include "../foenix/uart16550.h" /* Serial port */
#include "../foenix/a2560u.h"
#include "a2560u_bios.h"

/* Prototypes ****************************************************************/

void a2560u_init_lut0(void);

/* Interrupt */
void irq_mask_all(uint16_t *save);
void irq_add_handler(int id, void *handler);
void a2560u_irq_bq4802ly(void);
void a2560u_irq_vicky(void);
void a2560u_irq_ps2kbd(void);
void a2560u_irq_ps2mouse(void);


/* Implementation ************************************************************/

void a2560u_bios_init(void)
{
	a2560u_init();
}


/* Video  ********************************************************************/
#define VICKY_VIDEO_MODE_FLAG (1<<13) /* In the TOS video mode, indicates this is a VICKY mode */
uint32_t a2560u_fb_size;

static int videl_to_foenix_mode(uint16_t videlmode)
{
    return videlmode & ~VICKY_VIDEO_MODE_FLAG;
}


static int foenix_to_videl_mode(uint16_t foenixmode)
{
    return foenixmode | VICKY_VIDEO_MODE_FLAG;
}

#if CONF_WITH_A2560U_SHADOW_FRAMEBUFFER

/* To speed up the copy from RAM to VRAM we manage a list of dirty cells */
#define A2560U_DIRTY_CELLS_SIZE 1024
volatile struct
{
    uint16_t writer;
    uint16_t reader;
    uint16_t full_copy; /* Set this to non-zero to copy the whole frame buffer and flush the ring buffer */
    uint8_t *cells[A2560U_DIRTY_CELLS_SIZE];
} a2560u_bios_dirty_cells;


void a2560u_bios_mark_screen_dirty(void)
{
    a2560u_bios_dirty_cells.full_copy = -1;
}


void a2560u_bios_mark_screen_dirty(void)
{
    a2560u_bios_dirty_cells.full_copy = -1;
}

#endif


void a2560u_bios_screen_init(void)
{
    KDEBUG(("a2560u_bios_screen_init\n"));
    vicky2_init();

    /* Setup VICKY interrupts handler (VBL, HBL etc.) */
    vblsem = 0;
#if CONF_WITH_A2560U_SHADOW_FRAMEBUFFER
    a2560u_bios_dirty_cells.writer = a2560u_bios_dirty_cells.reader = a2560u_bios_dirty_cells.full_copy = 0;
#endif

    a2560u_irq_set_handler(INT_SOF_A, int_vbl);
}


void a2560u_bios_get_current_mode_info(uint16_t *planes, uint16_t *hz_rez, uint16_t *vt_rez)
{
    FOENIX_VIDEO_MODE mode;

    vicky2_read_video_mode(&mode);
    *planes = 8; /* We have 8bits per pixel */
    *hz_rez = mode.w;
    *vt_rez = mode.h;
}


uint8_t *a2560u_bios_physbase(void)
{
    KDEBUG(("a2560u_bios_physbase: %p\n", vicky2_get_bitmap_address(0) + VRAM_Bank0));
    return vicky2_get_bitmap_address(0) + VRAM_Bank0;
}


int16_t a2560u_bios_vmontype(void)
{
    KDEBUG(("a2560u_bios_vmontype\n"));
    return MON_VGA; /* VGA. 5 (DVI) would be more correct but is only known for CT60/Radeon so probably not known about by a lot of software */
}


int32_t a2560u_bios_vgetsize(int16_t videlmode)
{
    int foenix_mode_nr;
    FOENIX_VIDEO_MODE *foenix_mode;

    foenix_mode_nr = videl_to_foenix_mode(videlmode);
    foenix_mode = (FOENIX_VIDEO_MODE*)&foenix_video_modes[foenix_mode_nr];
    KDEBUG(("a2560u_bios_vgetsize returns %ld\n", (uint32_t)foenix_mode->w * (uint32_t)foenix_video_modes->h));
    return  (uint32_t)foenix_mode->w * (uint32_t)foenix_video_modes->h; /* 1 byte = 1 pixel on the Foenix, easy */
}


uint32_t a2560u_bios_calc_vram_size(void)
{
    FOENIX_VIDEO_MODE mode;
    vicky2_read_video_mode(&mode);
    KDEBUG(("a2560u_bios_calc_vram_size returns mode:%d, size=%ld\n", mode.id, mode.w * mode. h + EXTRA_VRAM_SIZE));
    return (uint32_t)mode.w * (uint32_t)mode. h + EXTRA_VRAM_SIZE;
}


/* Also acts as Vsetscreen */
void a2560u_bios_setrez(int16_t rez, int16_t mode)
{
    if (rez != FALCON_REZ)
        return;

    KDEBUG(("a2560u_bios_setrez(%d, %d)\n", rez, mode));
    vicky2_set_video_mode(videl_to_foenix_mode(mode));
}


uint16_t a2560u_bios_vsetmode(int16_t mode)
{
    FOENIX_VIDEO_MODE fm;
    KDEBUG(("a2560u_bios_vsetmode(%d)\n",mode));
    
    if (mode != -1 && (mode & VICKY_VIDEO_MODE_FLAG))
        vicky2_set_video_mode(mode);

    vicky2_read_video_mode(&fm);
    KDEBUG(("a2560u_bios_vsetmode returns %d\n", foenix_to_videl_mode(fm.id)));
    return foenix_to_videl_mode(fm.id);
}


void a2560u_bios_vsetrgb(int16_t index,int16_t count,const uint32_t *rgb)
{
    int i;
    COLOR32 fc; /* Foenix colour */
    uint8_t *argb;

    for (i = index; i <= count; i++) {
        argb = (uint8_t*)&rgb[i];
        fc.red = argb[1];
        fc.green = argb[2];
        fc.blue = argb[3];
        fc.alpha = 0xff;
        vicky2_set_lut_color(0, i, *((uint32_t*)&fc));
    }
}


void a2560u_bios_vgetrgb(int16_t index,int16_t count,uint32_t *rgb) {
    int i;
    COLOR32 fc; /* Foenix colour */
    uint8_t *argb;

    for (i = index; i <= count; i++) {
        vicky2_get_lut_color(0, i, &fc);
        argb = (uint8_t*)&rgb[i];
        argb[1] = fc.red;
        argb[2] = fc.green;
        argb[3] = fc.blue;
    }
}


/* Serial port ***************************************************************/

uint32_t a2560u_bios_bcostat1(void)
{
    return uart16550_can_put(UART0);
}

void a2560u_bios_bconout1(uint8_t byte)
{
    uart16550_put(UART0,&byte, 1);
}

void a2560u_irq_com1(void); // Event handler in a2560u_s.S

void a2560u_bios_rs232_init(void) {
    // The UART's base settings are setup earlier
    uart16550_rx_handler = push_serial_iorec;
    setexc(INT_COM1_VECN, (uint32_t)a2560u_irq_com1);
    a2560u_irq_enable(INT_COM1);
    uart16550_rx_irq_enable(UART0, true);
}

/* For being able to translate settings of the ST's MFP68901 we need this */
static const uint16_t mfp_timer_prediv[] = { 0,4,10,16,50,64,100,200 };
#define MFP68901_FREQ 2457600 /* The ST's 68901 MFP is clocked at 2.4576MHZ */

void a2560u_bios_xbtimer(uint16_t timer, uint16_t control, uint16_t data, void *vector)
{
    uint32_t frequency;
    uint32_t timer_clock;

    if (timer > 3)
        return;

    /* Read the intent of the caller */
    if (timer == 2)
        control >>= 4; /* TCDCR has control bit for Timer C and D but D has bits 4,5,6 */
    if (control & 0x8)
    {
        /* We only support "delay mode. Other modes have bit 3 set */
        KDEBUG(("Not supported MFP timer %d mode %04x\n", 'A' + timer, control));
        return;
    }
    /* Quantity of 2.4576MHz ticks before the timer fires */
    frequency = mfp_timer_prediv[frequency];
    if (data != 0)
        frequency *= data;

    /* Convert that according to our timer's clock */
    timer_clock = timer == 3 ? vicky_vbl_freq : 20000000/*TODO use cpu_freq*/;
    frequency = (frequency * timer_clock) / MFP68901_FREQ;

    a2560u_set_timer(timer, frequency, false, vector);
}


/* Calibration ***************************************************************/

uint32_t calibration_interrupt_count;
uint32_t calibration_loop_count;
void a2560u_bios_irq_calibration(void);
void a2560u_run_calibration(void);

void a2560u_bios_calibrate_delay(uint32_t calibration_time)
{
    uint16_t masks[IRQ_GROUPS];
    uint32_t old_timer_vector;

    calibration_interrupt_count = 0;
    calibration_loop_count = calibration_time;

    a2560u_debugnl("a2560u_bios_calibrate_delay(0x%ld)", calibration_time);
    /* We should disable timer 0 now but we really don't expect that anything uses it during boot */

    /* Backup all interrupts masks because a2560u_run_calibration will mask everything. We'll need to restore */
    a2560u_irq_mask_all(masks);

    /* Setup timer for 960Hz, same as what EmuTOS for ST does with using MFP Timer D (UART clock baud rate generator) for 9600 bauds */
    old_timer_vector = setexc(INT_TIMER0_VECN, -1L);
    a2560u_set_timer(0, 960, true, a2560u_bios_irq_calibration);

    /* This counts the number of wait loops we can do in about 100ms */
    a2560u_run_calibration();

    /* Restore previous timer vector */
    setexc(INT_TIMER0_VECN, old_timer_vector);

    /* Restore interrupt masks */
    a2560u_irq_restore(masks);

    a2560u_debugnl("loopcount_1_msec (old)= 0x%08lx, calibration_interrupt_count = %ld", loopcount_1_msec, calibration_interrupt_count);
    /* See delay.c for explaination */
    if (calibration_interrupt_count)
        loopcount_1_msec = (calibration_time * 24) / (calibration_interrupt_count * 25);

    a2560u_debugnl("loopcount_1_msec (new)= 0x%08lx", loopcount_1_msec);
}


/* PS/2 setup  ***************************************************************/
#include "../foenix/ps2.h"

static void *balloc_proxy(size_t howmuch)
{
    return balloc_stram(howmuch, false);
}

void a2560u_bios_kbd_init(void)
{
    ps2_config.counter      = (uint32_t*)&frclock; /* FIXME we're using the VBL counter */
    ps2_config.counter_freq = 60;
    ps2_config.malloc       = balloc_proxy;
    ps2_config.os_callbacks.on_key_down = kbd_int;
    ps2_config.os_callbacks.on_key_up   = kbd_int;
    ps2_config.os_callbacks.on_mouse    = call_mousevec;

    a2560u_kbd_init();
}


/* SD card *******************************************************************/

#include "spi.h"


/* Nothing needed there, it's all handled by GAVIN */
void spi_clock_sd(void) { }
void spi_clock_mmc(void) { }
void spi_clock_ident(void) { }
void spi_cs_assert(void) { }
void spi_cs_unassert(void) { }

void spi_initialise(void)
{
    /* We use plain SPI and EmuTOS's SD layer on top of it */
    sdc_controller->transfer_type = SDC_TRANS_DIRECT;
}


static uint8_t clock_byte(uint8_t value)
{
    sdc_controller->data = value;
    sdc_controller->transfer_control = SDC_TRANS_START;
    while (sdc_controller->transfer_status & SDC_TRANS_BUSY)
        ;
    return sdc_controller->data;
}


void spi_send_byte(uint8_t c)
{
    (void)clock_byte(c);
}


uint8_t spi_recv_byte(void)
{
    return clock_byte(0xff);
}


/* Text mode support *********************************************************/
#include "font.h"
#include "lineavars.h"
#include "biosext.h"

/* Line-A variables */
extern uint16_t v_col_fg; /* Font background */
extern uint16_t v_col_bg; /* Font foreground */
extern uint16_t v_cur_ad; /* Current cursor address */

static const uint16_t text_palette[32] =
{
/*  0xHHLL, 0xHHLL
 *  0xGGBB, 0xAARR */
#define BLUE_THEME 0
#define GREEN_THEME 1
#if BLUE_THEME
    0x2b4f, 0xff0e, /* A2560 background */
#elif GREEN_THEME
    0x1100, 0xff00,
#else
	0x0000, 0xFF00,	/* Black (transparent) */
#endif
	0x0000, 0xFF80, /* Mid-Tone Red */
	0x8000, 0xFF00, /* Mid-Tone Green */
	0x8000, 0xFF80, /* Mid-Tone Yellow */
	0x0080, 0xFF00, /* Mid-Tone Blue */
	0x5500, 0xFFAA, /* Mid-Tone Orange */
	0x8080, 0xFF00, /* Mid-Tone Cian */
	0x8080, 0xFF80, /* 50% Grey */
	0x5555, 0xFF55, /* Dark Grey */
    0x5555, 0xFFFF, /* Bright Red */
	0xFF55, 0xFF55, /* Bright Green */
	0xFF55, 0xFFFF, /* Bright Yellow */
	0x55FF, 0xFF55, /* Bright Blue */
	0x7FFF, 0xFFFF, /* Bright Orange */
	0xFFFF, 0xFF55, /* Bright Cyan */
#if BLUE_THEME
	0x55BB, 0xFF55, /* A2560 light blue */
#elif GREEN_THEME
    0xee00, 0xff00, /* A2560 light green */
#else
    0xFFFF, 0xFFFF 	/* White */
#endif
};


static void a2560u_bios_load_font(void)
{
    int ascii;
    int i;

    uint8_t *dst = (uint8_t*)VICKY_FONT;

    for (ascii = 0; ascii < 256; ascii++)
    {
        uint8_t *src = char_addr(ascii);

        /* Character 0 is the cursor (filled block) */
        if (ascii && src != 0L)
        {
            for (i = 0; i < v_cel_ht; i++)
            {
                *dst++ = *src;
                src += v_fnt_wr;
            }
        }
        else
        { 
            for (i = 0; i < v_cel_ht ; i++)
                *dst++ = 0xff;
        }
    }
}


void a2560u_bios_text_init(void)
{
    a2560u_bios_load_font();
    vicky2_set_text_lut(text_palette, text_palette);

    int i;
    volatile uint8_t *c = (volatile uint8_t*)VICKY_TEXT_COLOR;
    volatile uint8_t *t = (uint8_t*)VICKY_TEXT;
    uint8_t color = (uint8_t)(v_col_fg << 4 | v_col_fg);

    for (i = 0 ; i < VICKY_TEXT_SIZE ; i++)
    {
         *c++ = color;
         *t++ = ' ';
    }

    /* Set cursor */
    R32(VICKY_A_CURSOR_CTRL) = 0;
    //R32(VICKY_A_CURSOR_CTRL) &= ~VICKY_CURSOR_CHAR;
    R32(VICKY_A_CURSOR_CTRL) |=
        (((uint32_t)0x00L) << 16) /* 0 is a filled cell */
        | 6; /* Flash quickly */

    /* Enable text mode. Can't have text mode and gfx without overlay otherwise it crashes.
     * If we have overlay, then the inverse video doesn't work as the background is transparent. */
    R32(VICKY_CTRL) &= ~(VICKY_A_CTRL_GFX|VICKY_A_CTRL_BITMAP);
    R32(VICKY_CTRL) |= VICKY_A_CTRL_TEXT;
    vicky2_hide_cursor();
}

#endif /* MACHINE_A2560U */
