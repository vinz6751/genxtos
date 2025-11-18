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

#if defined(MACHINE_A2560U) || defined(MACHINE_A2560X) || defined(MACHINE_A2560K) || defined(MACHINE_GENX) || defined(MACHINE_A2560M)

#include "portab.h"
#include "aciavecs.h"
#include "vectors.h"
#include "tosvars.h"
#include "bios.h"
#include "biosdefs.h"
#include "processor.h"
#include "biosext.h"            /* for cache control routines */
#include "gemerror.h"
#include "ikbd.h"               /* for call_mousevec() */
#include "keyboard.h"
#include "screen.h"
#include "delay.h"
#include "asm.h"
#include "conout.h"
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
#include "../foenix/a2560.h"
#include "../foenix/interrupts.h"
#include "../foenix/mpu401.h"
#include "../foenix/shadow_fb.h"
#include "../foenix/timer.h"
#include "../foenix/vicky2.h"
#include "a2560_bios.h"
#include "../foenix/regutils.h"

bool a2560_bios_sfb_is_active;

/* Prototypes ****************************************************************/

void a2560_init_lut0(void);


/* Implementation ************************************************************/

void a2560_bios_init(void)
{
	a2560_init(warm_magic != WARM_MAGIC);
}


void a2560_bios_enable_irqs(void)
{
    a2560_timer_enable(HZ200_TIMER_NUMBER,true);
    a2560_irq_enable(INT_SOF_A);
}

/* Video  ********************************************************************/
#define VICKY_VIDEO_MODE_FLAG (1<<13) /* In the TOS video mode, indicates this is a VICKY mode */

static int videl_to_foenix_mode(uint16_t videlmode)
{
    return videlmode & ~VICKY_VIDEO_MODE_FLAG;
}


static int foenix_to_videl_mode(uint16_t foenixmode)
{
    return foenixmode | VICKY_VIDEO_MODE_FLAG;
}


void a2560_bios_screen_init(void)
{
    KDEBUG(("a2560_bios_screen_init\n"));

    vicky2_init();

    /* Setup VICKY interrupts handler (VBL, HBL etc.) */
    vblsem = 0;

#if CONF_WITH_A2560U_SHADOW_FRAMEBUFFER
    a2560_sfb_init();
#endif

    a2560_irq_set_handler(INT_SOF_A, int_vbl);
    KDEBUG(("a2560_bios_screen_init exiting\n"));
}


void a2560_bios_get_current_mode_info(uint16_t *planes, uint16_t *hz_rez, uint16_t *vt_rez)
{
    FOENIX_VIDEO_MODE mode;

    vicky2_read_video_mode(vicky, &mode);
    *planes = mode.bpp; /* We have 8bits per pixel */
    *hz_rez = mode.w;
    *vt_rez = mode.h;
    KDEBUG(("a2560_bios_get_current_mode_info setting hz_rez:%d vt_rez:%d from mode %d\n", *hz_rez, *vt_rez, mode.id));
}


uint8_t *a2560_bios_physbase(void)
{
    KDEBUG(("a2560_bios_physbase: %p\n", vicky2_get_bitmap_address(vicky, 0) + VRAM_Bank0));
    return vicky2_get_bitmap_address(vicky, 0) + VRAM_Bank0;
}


int16_t a2560_bios_vmontype(void)
{
    KDEBUG(("a2560_bios_vmontype\n"));
    return MON_VGA; /* VGA. 5 (DVI) would be more correct but is only known for CT60/Radeon so probably not known about by a lot of software */
}


int32_t a2560_bios_vgetsize(int16_t videlmode)
{
    int foenix_mode_nr;
    FOENIX_VIDEO_MODE *foenix_mode;

    foenix_mode_nr = videl_to_foenix_mode(videlmode);
    foenix_mode = (FOENIX_VIDEO_MODE*)&vicky->video_modes[foenix_mode_nr];
    KDEBUG(("a2560_bios_vgetsize returns %ld\n", (uint32_t)foenix_mode->w * (uint32_t)foenix_mode->h));
    return  (uint32_t)foenix_mode->w * (uint32_t)foenix_mode->h; /* 1 byte = 1 pixel on the Foenix, easy */
}


uint32_t a2560_bios_calc_vram_size(void)
{
    FOENIX_VIDEO_MODE mode;
    vicky2_read_video_mode(vicky, &mode);
    KDEBUG(("a2560_bios_calc_vram_size returns mode:%d, size=%ld\n", mode.id, mode.w * mode. h + EXTRA_VRAM_SIZE));
    return (uint32_t)mode.w * (uint32_t)mode. h + EXTRA_VRAM_SIZE;
}


/* Also acts as Vsetscreen */
void a2560_bios_setrez(int16_t rez, int16_t mode)
{
    if (rez != FALCON_REZ)
        return;

    KDEBUG(("a2560_bios_setrez(%d, %d)\n", rez, mode));
    vicky2_set_video_mode(vicky, videl_to_foenix_mode(mode));
}


uint16_t a2560_bios_vsetmode(int16_t mode)
{
    FOENIX_VIDEO_MODE fm;
    KDEBUG(("a2560_bios_vsetmode(%d)\n",mode));

    if (mode != -1 && (mode & VICKY_VIDEO_MODE_FLAG))
        vicky2_set_video_mode(vicky, mode);

    vicky2_read_video_mode(vicky, &fm);
    KDEBUG(("a2560_bios_vsetmode returns %d\n", foenix_to_videl_mode(fm.id)));
    return foenix_to_videl_mode(fm.id);
}


void a2560_bios_vsetrgb(int16_t index,int16_t count,const uint32_t *rgb)
{
    int i;

    for (i = index; i <= count; i++) {
#ifdef MACHINE_A2560U
        COLOR32 fc; /* Foenix colour */
        uint8_t *argb;
        argb = (uint8_t*)&rgb[i];
        fc.red = argb[1];
        fc.green = argb[2];
        fc.blue = argb[3];
        fc.alpha = 0xff;
        vicky2_set_lut_color(vicky, 0, i, *((uint32_t*)&fc));
#elif defined(MACHINE_A2560U)
    vicky2_set_lut_color(vicky, 0, i, rgb)[i];
#endif
    }
}


void a2560_bios_vgetrgb(int16_t index,int16_t count,uint32_t *rgb) {
    int i;
    COLOR32 fc; /* Foenix colour */
    uint8_t *argb;

    for (i = index; i <= count; i++) {
        vicky2_get_lut_color(vicky, 0, i, &fc);
        argb = (uint8_t*)&rgb[i];
        argb[1] = fc.red;
        argb[2] = fc.green;
        argb[3] = fc.blue;
    }
}


/* Serial port ***************************************************************/

uint32_t a2560_bios_bcostat1(void)
{
    return uart16550_can_put((UART16550*)UART1);
}

void a2560_bios_bconout1(uint8_t byte)
{
    uart16550_put((UART16550*)UART1,&byte, 1);
}

void a2560_irq_com1(void); // Event handler in a2560_s.S

void a2560_bios_rs232_init(void) {
    a2560_debugnl("a2560_bios_rs232_init");
    // The UART's base settings are setup earlier
    uart16550_rx_handler = push_serial_iorec;
    setexc(INT_COM1_VECN, (uint32_t)a2560_irq_com1);
    a2560_irq_enable(INT_COM1);
    uart16550_rx_irq_enable((UART16550*)UART1, true);
}

/* This does not perfectly emulate the MFP but may enough */
uint32_t a2560_bios_rsconf1(int16_t baud_code, EXT_IOREC *iorec, int16_t ctrl, int16_t ucr, int16_t rsr, int16_t tsr, int16_t scr)
{
    static const int16_t baud_codes[] = {
        UART16550_19200BPS, UART16550_9600BPS, UART16550_4800BPS, UART16550_3600BPS,
        UART16550_2400BPS, UART16550_2000BPS, UART16550_1800BPS, UART16550_1200BPS,
        UART16550_600BPS, UART16550_300BPS, UART16550_200BPS, UART16550_150BPS,
        // This is not TOS compliant but we need to be able to use higher speeds than 19200bps...
        // 12               13                  14                   15
        UART16550_38400BPS, UART16550_57600BPS, UART16550_115200BPS, UART16550_230400BPS
    };
    // Note that 230400bps is not currently supported because it requires setting the "High Speed"
    // bit of the SuperIO and I'm not sure how to do that

    static const uint8_t dsize[] = {
        UART16550_8D, UART16550_7D, UART16550_6D, UART16550_5D
    };
    uint8_t flags;
    uint8_t data_size;
    uint8_t data_format;

    if (baud_code == -2)
    {
        return iorec->baudrate;
    }
    else if (baud_code >= 0) {
        if (baud_code > ARRAY_SIZE(baud_codes)) {
            KDEBUG(("a2560_bios_rsconf1 setting invalid baud specification %d\n", baud_code));
        }
        else {
            KDEBUG(("[DISABLED] a2560_bios_rsconf1 setting speed %d bps (code: %d)\n", baud_codes[baud_code], baud_code));
            uart16550_set_bps((UART16550*)UART1, baud_codes[baud_code]);
            iorec->baudrate = baud_code;
        }
    }

    flags = 0;
    data_size = dsize[(ucr & 0x60) >> 5];
    data_format = (ucr & 0x18) >> 3;

    if (ucr != -1)
    {
        // Parity
        if (ucr & 2)
            flags |= ucr & 1 ? UART16550_ODD : UART16550_EVEN;
        // Data size
        flags |= data_size;
        // Stop bits
        if (data_size != UART16550_5D)
        {
            if (data_format == 3/* 1 start 2 stops*/)
                flags |= UART16550_2S;
        }
        else if (data_format == 2/* 1 start 1.5 stop */)
                flags |= UART16550_1_5S;

        uart16550_set_line((UART16550*)UART1, flags);
        KDEBUG(("a2560_bios_rsconf1 setting flags %x\n", flags));
    }


    // RTS/CTS and XON/XOFF not supported ! A2560U doesn't have RTS/CTS pins connected anyway
    // TODO for machines having SuperIO
    return 0L; // TODO.
}


/* Timers *********************************************************************/

/* For being able to translate settings of the ST's MFP68901 we need this */
static const uint16_t mfp_timer_prediv[] = { 0,4,10,16,50,64,100,200 };
#define MFP68901_FREQ 2457600 /* The ST's 68901 MFP is clocked at 2.4576MHZ */

void a2560_bios_xbtimer(uint16_t timer, uint16_t control, uint16_t data, void *vector)
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

    a2560_set_timer(timer, frequency, false, vector);
}

/* PS/2 setup  ***************************************************************/

#include "../foenix/trap_bindings.h"

void a2560_bios_kbd_init(void)
{
    /* We're using the VBL counter: that may depend on the resolution, if it changes,
     * and of the VBLs are blocked, the counter will not work. But after init, when everything
     * is setup and we're only reacting to interupts, it shouldn't matter. */
    fnx_kbd_init((uint32_t*)&frclock, 60);
    fnx_ps2_set_key_up_handler(kbd_int);
    fnx_ps2_set_key_down_handler(kbd_int);
    fnx_ps2_set_mouse_handler(call_mousevec);
}


/* SD Card: the driver is in spi_gavin */

/* Text mode support *********************************************************/
#include "font.h"
#include "lineavars.h"
#include "biosext.h"

static void a2560_bios_load_font(void);

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

void a2560_bios_sfb_setup(uint8_t *addr, uint16_t text_cell_height)
{
    a2560_debugnl("a2560_bios_sfb_setup(%p,%d)", addr, text_cell_height);
    a2560_sfb_setup(addr, text_cell_height);
    a2560_bios_sfb_is_active = true;
}

extern const CONOUT_DRIVER a2560_conout_text;
extern const CONOUT_DRIVER a2560_conout_bmp;

CONOUT_DRIVER *a2560_bios_get_conout(void)
{
    CONOUT_DRIVER *driver;
    driver = NULL;

    a2560_debugnl("a2560_bios_get_conout");

    if (v_cel_ht == 8 && CONF_WITH_A2560_TEXT_MODE)
    {
        a2560_debugnl("a2560_bios_get_conout selected the TEXT mode driver");
        /* VICKY helps us with the 8x8 font and cursor blinking */
        a2560_bios_sfb_is_active = false;
        driver =  (CONOUT_DRIVER*)&a2560_conout_text;
    }
# if CONF_WITH_A2560U_SHADOW_FRAMEBUFFER
    else
    {
        a2560_debugnl("a2560_bios_get_conout selected the BITMAP driver %p", v_bas_ad);
        /* Use the shadow framebuffer */
        driver = (CONOUT_DRIVER*)&a2560_conout_bmp;
        a2560_sfb_setup(v_bas_ad, v_cel_ht);
    }
#endif
    if (driver == NULL)
        panic("Cannot select conout driver\n");

    return driver;
}

/** Perform initialisation for the screen to be ready to support text mode (vs bitmap text)
 * This should setup a font, the cursor, the colors of the text mode.
 * It doesn't need to clear the screen.
 * I'm not sure if it should set the video mode, as this may compete with screen_init_mode()
 */
void a2560_bios_text_init(void)
{
    a2560_debugnl("a2560_bios_text_init(%p)", vicky);
    a2560_bios_load_font();

    a2560_debugnl("a2560_bios_text_init: vicky2_set_text_lut");
    vicky2_set_text_lut(vicky, text_palette, text_palette);

    int i;
    volatile uint8_t *c = vicky->text_memory->color;
    volatile uint8_t *t = vicky->text_memory->text;
    uint8_t color = (uint8_t)(v_col_fg << 4 | v_col_fg);

    a2560_debugnl("color_mem:%p text_mem:%p",c,t);

    #if 0 // that shouldn't be necessary. The terminal will be cleared later */
    for (i = 0 ; i < VICKY_TEXT_SIZE ; i++)
    {
         *c++ = color;
         *t++ = ' ';
    }
    #endif

    /* Set cursor */
    vicky->ctrl->cursor_control = 0;
    //vicky->cursor_control  &= ~VICKY_CURSOR_CHAR;
    vicky->ctrl->cursor_control  |=
        (((uint32_t)0x00L) << 16) /* 0 is a filled cell */
        | 6; /* Flash quickly */

    /* Enable text mode. Can't have text mode and gfx without overlay otherwise it crashes.
     * If we have overlay, then the inverse video doesn't work as the background is transparent. */
    if (vicky->bitmap)
        vicky->ctrl->control &= ~(VICKY_CTRL_GFX|VICKY_CTRL_BITMAP);
    vicky->ctrl->control |= VICKY_CTRL_TEXT;
    vicky2_hide_cursor(vicky);

    a2560_debugnl("a2560_bios_text_init exiting");
}


static void a2560_bios_load_font(void)
{
    int ascii;
    int i;

    uint8_t *dst = vicky->font_memory->mem;
    a2560_debugnl("a2560_bios_load_font loading found to %p", dst);

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


/* MIDI support */
#if CONF_WITH_MPU401

static ULONG get_timeout_timer(void) {
    return hz_200;
}

void a2560_bios_midi_init(void) {
    a2560_midi_init(get_timeout_timer, 1000 / timer_ms/*1 second*/);
#ifdef ENABLE_KDEBUG
    if (!kbdvecs.midivec)
        KDEBUG(("a2560_bios_midi_init: kbdvecs.midivec is NULL, expect crashes !\n"));
#endif
    mpu401_rx_handler = kbdvecs.midivec;
}

uint32_t a2560_bios_bcostat3(void)
{
    return mpu401_can_write();
}

void a2560_bios_bconout3(uint8_t byte)
{
    mpu401_write(byte);
}

#endif // CONF_WITH_MPU401

#endif /* defined(MACHINE_A2560U) || defined(MACHINE_A2560K) || defined(MACHINE_A2560M) || defined(MACHINE_A2560X) || defined(MACHINE_GENX) */
