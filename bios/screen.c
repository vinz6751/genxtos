/*
 * screen.c - XBIOS-level screen routines
 *
 * Copyright (C) 2001-2025 The EmuTOS development team
 *
 * Authors:
 *  MAD   Martin Doering
 *  THH   Thomas Huth
 *  LVL   Laurent Vogel
 *  PES   Petr Stehlik
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#define ENABLE_KDEBUG

#include "emutos.h"
#include "asm.h"
#include "bios.h"
#include "biosext.h" // video_ram_addr
#include "biosmem.h"
#include "has.h"
#include "linea.h" // V_REZ_HZ / 
#include "bdosbind.h" // Srealloc() FIXME layering breakage: XBIOS calling GEMDOS
#include "screen.h"
#include "tosvars.h"
#include "vt52.h"
#include "amiga.h"
#include "lisa.h"
#include "nova.h"
#include "screen_atari.h"
#include "screen_tt.h"
#include "videl.h"
#include "xbiosbind.h" /* for Srealloc, which is a layering breakage ! (BIOS calling BDOS */
#include "a2560_bios.h"

#define HAS_VIDEO_HARDWARE (defined(MACHINE_AMIGA) || CONF_WITH_ATARI_VIDEO || defined(MACHINE_A2560U) || defined(MACHINE_A2560K) || defined(MACHINE_A2560M) || defined(MACHINE_A2560X) || defined(MACHINE_GENX))

LONG video_ram_size;        /* these are used by Srealloc() */
void *video_ram_addr;

char rez_was_hacked;

static void setup_video_ram(void);
static ULONG calc_vram_size(void);
static void screen_init_services(UWORD planes, UWORD xrez, UWORD yrez);

// Select the driver
#if CONF_WITH_ATARI_VIDEO
 #define screen_driver screen_driver_atari
#elif defined(MACHINE_AMIGA)
 #define screen_driver screen_driver_amiga
#elif defined(MACHINE_LISA)
 #define screen_driver screen_driver_lisa
#elif defined(MACHINE_A2560U) || defined(MACHINE_A2560K)
 extern const SCREEN_DRIVER a2560_screen_driver_vicky2;
 #define screen_driver a2560_screen_driver_vicky2
#elif defined(MACHINE_A2560M)
 extern const SCREEN_DRIVER a2560_screen_driver_vicky3;
 #define screen_driver a2560_screen_driver_vicky3
#elif defined(MACHINE_A2560X) || defined(MACHINE_GENX)
 extern const SCREEN_DRIVER a2560_screen_driver_1bpp;
 #define screen_driver a2560_screen_driver_1bpp
 #endif


void screen_init(void) {
    /* Initialize the video mode and palette. The video memory address will be done later. */
    screen_driver.init();

    rez_was_hacked = FALSE; /* initial assumption */

    /* Setup the video RAM / allocate it if necessary */
    KDEBUG(("setup_video_ram()\n"));
    setup_video_ram();

    /* Make the video hardware use that video REM */
    KDEBUG(("screen_setphys(%p)\n",video_ram_addr));
    screen_setphys(video_ram_addr);
}

/*
 * Check specified mode/rez to see if we should change; used in early
 * emudesk.inf processing.  We only indicate a change if all of the
 * following are true:
 *  . the resolution is changeable
 *  . the specified & current values are the same type (both modes
 *    or both rezs)
 *  . the specified value differs from the current value
 * If these are all true, we return the new mode/rez; otherwise we
 * return zero.
 *
 * Mode/rez values are encoded as follows:
 *      0xFFnn: ST/TT resolution nn
 *      otherwise, Falcon mode value
 */
WORD check_moderez(WORD moderez)
{
    if (!screen_can_change_resolution())
        return 0;

    return screen_driver.check_moderez(moderez);
}

/*
 * Initialise palette registers
 * This routine is also used by resolution change
 */
void initialise_palette_registers(WORD rez, WORD mode)
{
    screen_driver.initialise_palette_registers(rez, mode);
}


/* Initialize the video memory (allocate if necessary). Expects the video mode to be already set because calc_vram_size depends on it.
 * Note that his is only called at startup. For subsequent resolution changes, the process is much more painful for the caller.
 */
static void setup_video_ram(void)
{
    LONG vram_size;
    UBYTE *vram_address;
    MAYBE_UNUSED(vram_size);

#if CONF_VRAM_ADDRESS
    vram_size = VIDEO_RAM_SIZE_UNSPECIFIED;
    vram_address = (UBYTE *)CONF_VRAM_ADDRESS;
#else
    vram_size = calc_vram_size();
    /* videoram is placed just below the top of the RAM (phystop) */
    vram_address = balloc_stram(vram_size, TRUE);
#endif /* CONF_VRAM_ADDRESS */

    video_ram_size = vram_size;     /* these are used by Srealloc() */
    video_ram_addr = vram_address;

    /* set the v_bas_ad system variable */
    v_bas_ad = video_ram_addr;
    KDEBUG(("v_bas_ad = %p, vram_size = %ld\n", v_bas_ad, vram_size));
}

/*
 * Mark resolution as hacked
 *
 * called by bios_init() if a special video mode (Nova support, Hatari
 * cartridge extended VDI) has altered key lineA variables
 */
void screen_set_rez_hacked(void)
{
    rez_was_hacked = TRUE;

    screen_init_services(v_planes, V_REZ_HZ, V_REZ_VT);
}

/*
 * Check if resolution can be changed
 *
 * returns 1 iff TRUE
 */
WORD screen_can_change_resolution(void)
{
    if (rez_was_hacked)
        return FALSE;

    return screen_driver.can_change_resolution();
}


/* get monitor type (same encoding as VgetMonitor()) */
WORD get_monitor_type(void)
{
    return screen_driver.get_monitor_type();
}


/*
 * calculate VRAM size based on video hardware
 *
 * note: all versions of Atari TOS overallocate memory; we do the same
 * because some programs (e.g. NVDI) rely on this and write past what
 * should be the end of screen memory.
 */
static ULONG calc_vram_size(void)
{
    return screen_driver.calc_vram_size();
}


/*
 * used by vdi_v_opnwk()
 *
 * returns the palette (number of colour choices) for the current hardware
 */
WORD get_palette(void)
{
    return screen_driver.get_number_of_colors_nuances();
}


/* returns 'standard' pixel sizes */
void get_std_pixel_size(WORD *width,WORD *height)
{
    *width = (V_REZ_HZ < 640) ? 556 : 278;  /* magic numbers as used */
    *height = (V_REZ_VT < 400) ? 556 : 278; /*  by TOS 3 & TOS 4     */
}

/*
 * used by vdi_v_opnwk()
 *
 * pixel sizes returned here affect (at least) how the following
 * are displayed:
 *  - the output from v_arc()/v_circle()/v_pieslice()
 *  - the size of gl_wbox in pixels
 *
 * we used to base the pixel sizes for ST(e) systems on exact screen
 * width and height values.  however, this does not work for enhanced
 * screens, such as Hatari's 'extended VDI screen' or add-on hardware.
 *
 * we now use some heuristics in the hope that this will cover the most
 * common situations.  unfortunately we cannot set the sizes based on
 * the value from getrez(), since this may be inaccurate for non-standard
 * hardware.
 */
void get_pixel_size(WORD *width,WORD *height)
{
    screen_driver.get_pixel_size(width, height);
}


/* hardware independent xbios routines */

const UBYTE *physbase(void)
{
#if HAS_VIDEO_HARDWARE
    return screen_driver.physbase();
#else
    /* No real physical screen, fall back to Logbase() */
    return logbase();
#endif
}


/* Set physical screen address */

void screen_setphys(const UBYTE *addr)
{
    screen_driver.setphys(addr);
}


UBYTE *logbase(void)
{
    return v_bas_ad;
}


WORD getrez(void)
{ /* TODO should that be improved */
#if CONF_WITH_ATARI_VIDEO
    return atari_getrez();
#else
    /* No video hardware, return the logical video mode */
    return sshiftmod;
#endif
}


/*
 * setscreen(): implement the Setscreen() xbios call
 *
 * implementation summary:
 *  . for all hardware:
 *      . sets the logical screen address from logLoc, iff logLoc > 0
 *      . sets the physical screen address from physLoc, iff physLoc > 0
 *  . for videl, if logLoc==0 and physLoc==0:
 *      . reallocates screen memory and updates logical & physical
 *        screen addresses
 *  . for all hardware:
 *      . sets the screen resolution iff 0 <= rez <= 7 (this includes
 *        setting the mode specified by 'videlmode' if appropriate)
 *      . reinitialises lineA and the VT52 console
 */
WORD setscreen(UBYTE *logLoc, const UBYTE *physLoc, WORD rez, WORD videlmode)
{
    return screen_driver.setscreen(logLoc, physLoc, rez, videlmode);
}


static void screen_init_services(UWORD planes, UWORD xrez, UWORD yrez)
{
    KDEBUG(("screen_init_services(%d, %d, %d)\n", planes, xrez, yrez));
    /* Temporarily halt VBL processing. We use --/++ rather than = 0/1 because during
     * boot, we call this function, but we don't want it to start VBL processing yet. */
     vblsem--;

     /* Re-initialize line-a, VT52 etc: */
     linea_init(planes, xrez, yrez);
     vt52_init();
 
     /* Restart VBL processing */
     vblsem++; 
}


void screen_init_services_from_mode_info(void)
{
    UWORD planes, xrez, yrez;

    KDEBUG(("screen_init_services_from_mode_info\n"));

    screen_driver.get_current_mode_info(&planes, &xrez, &yrez);
    screen_init_services(planes, xrez, yrez);
}

/** Schedule the palette change for the next VBL
 * The new palette is stored as colorptr
*/
void setpalette(const UWORD *palettePtr)
{
#ifdef ENABLE_KDEBUG
    int i, max;
    max = getrez() == ST_LOW ? 15 : getrez() == ST_MEDIUM ? FALCON_REZ : ST_MEDIUM;
    KDEBUG(("Setpalette("));
    for(i = 0 ; i <= max ; i++) {
        KDEBUG(("%03x", palettePtr[i]));
        if(i < 15)
            KDEBUG((" "));
    }
    KDEBUG((")\n"));
#endif
    /* the flip will be initiated during the next VBL */
    colorptr = palettePtr;
}


/**
 * Set the palette
 * 
 * This function is called by the VBL interrupt handler to set the palette.
 * It is used to set the palette for the Falcon and the Shifter.
 * It is also used to set the palette for the VICKY.
 * 
 * It is called after the Setpalette xbios call is made.
 */
void screen_do_set_palette(const UWORD *new_palette) {
    screen_driver.set_palette(new_palette);
}


/*
 * setcolor(): implement the Setcolor() xbios call
 *
 * note that this only sets the ST(e)-compatible palette registers.
 * on a TT, the h/w updates the corresponding TT palette registers
 * automagically.
 */
WORD setcolor(WORD colorNum, WORD color)
{
#if HAS_VIDEO_HARDWARE
    return screen_driver.setcolor(colorNum, color);
#else
    /* No hardware, fake return value */
    return 0;
#endif
}


void vsync(void)
{
    LONG a;
#if CONF_WITH_ATARI_VIDEO
    WORD old_sr = set_sr(0x2300);       /* allow VBL interrupt */
    /* Beware: as a side effect, MFP interrupts are also enabled.
     * So the MFP interruptions must be carefully initialized (or disabled)
     * before calling vsync().
     * This is ugly, but Atari TOS does the same.
     */
#endif

    a = frclock;
    while (frclock == a) {
#if USE_STOP_INSN_TO_FREE_HOST_CPU
        stop_until_interrupt();
#endif
        /* Wait */
    }

#if CONF_WITH_ATARI_VIDEO
    set_sr(old_sr);
#endif /* CONF_WITH_ATARI_VIDEO */
}

