/*
 * amiga.h - Amiga specific functions
 *
 * Copyright (C) 2013-2019 The EmuTOS development team
 *
 * Authors:
 *  VRI   Vincent Rivière
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#ifndef AMIGA_H
#define AMIGA_H

#ifdef MACHINE_AMIGA

struct IDE
{
    UBYTE filler00[4];
    UBYTE features; /* Read: error */
    UBYTE filler06[3];
    UBYTE sector_count;
    UBYTE filler0a[3];
    UBYTE sector_number;
    UBYTE filler0e[3];
    UBYTE cylinder_low;
    UBYTE filler12[3];
    UBYTE cylinder_high;
    UBYTE filler16[3];
    UBYTE head;
    UBYTE filler1a[3];
    UBYTE command; /* Read: status */
    UBYTE filler1e[4091];
    UBYTE control; /* Read: Alternate status */
    UBYTE filler1019[3];
    UBYTE address; /* Write: Not used */
    UBYTE filler02[4067];
    UWORD data;
};

#define ide_interface ((volatile struct IDE*)0x00da0000)

extern const UBYTE scancode_atari_from_amiga[128];
extern UWORD amiga_screen_width;
extern UWORD amiga_screen_width_in_bytes;
extern UWORD amiga_screen_height;
extern const UBYTE *amiga_screenbase;
extern UWORD *copper_list;
extern int has_gayle;

void amiga_startup(void);
void amiga_machine_detect(void);
const char *amiga_machine_name(void);
void amiga_autoconfig(void);
#if CONF_WITH_ALT_RAM
void amiga_add_alt_ram(void);
ULONG amiga_detect_ram(void *start, void *end, ULONG step);
#endif
ULONG amiga_initial_vram_size(void);
void amiga_screen_init(void);
WORD amiga_check_moderez(WORD moderez);
void amiga_get_current_mode_info(UWORD *planes, UWORD *hz_rez, UWORD *vt_rez);
void amiga_setphys(const UBYTE *addr);
const UBYTE *amiga_physbase(void);
WORD amiga_setcolor(WORD colorNum, WORD color);
void amiga_setrez(WORD rez, WORD videlmode);
void amiga_kbd_init(void);
void amiga_ikbd_writeb(UBYTE b);
void amiga_extra_vbl(void);
void amiga_clock_init(void);
ULONG amiga_getdt(void);

#if CONF_WITH_UAE
typedef ULONG uaelib_demux_t(ULONG fnum, ...);
extern uaelib_demux_t* uaelib_demux;
#define has_uaelib (uaelib_demux != NULL)

void amiga_uae_init(void);
void kprintf_outc_uae(int c);
#endif

void amiga_shutdown(void);
BOOL amiga_can_shutdown(void);

void amiga_floppy_init(void);
BOOL amiga_flop_detect_drive(WORD dev);
WORD amiga_floprw(UBYTE *buf, WORD rw, WORD dev, WORD sect, WORD track, WORD side, WORD count);
LONG amiga_flop_mediach(WORD dev);

void amiga_rs232_init(void);
BOOL amiga_rs232_can_write(void);
void amiga_rs232_writeb(UBYTE b);
void amiga_rs232_rbf_interrupt(void);

/* The following functions are defined in amiga2.S */

void amiga_init_keyboard_interrupt(void);
void amiga_vbl(void);
void amiga_int_5(void);

#endif /* MACHINE_AMIGA */

#endif /* AMIGA_H */
