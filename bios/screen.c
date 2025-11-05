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

/* #define ENABLE_KDEBUG */

#include "emutos.h"
#include "asm.h"
#include "bios.h"
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
#include "xbiosbind.h"
#include "a2560_bios.h"


#if CONF_WITH_VIDEL
LONG video_ram_size;        /* these are used by Srealloc() */
void *video_ram_addr;
#endif

/* This flavour allows the caller to specify the resolution */
static void screen_init_services(UWORD planes, UWORD xrez, UWORD yrez);


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

#ifdef MACHINE_AMIGA
    return amiga_check_moderez(moderez);
#endif

#if CONF_WITH_ATARI_VIDEO
    return atari_check_moderez(moderez);
#else
    return 0;
#endif
}

/*
 * Initialise palette registers
 * This routine is also used by resolution change
 */
void initialise_palette_registers(WORD rez, WORD mode)
{
#if CONF_WITH_ATARI_VIDEO
    UWORD mask;

    if (HAS_VIDEL || HAS_TT_SHIFTER || HAS_STE_SHIFTER)
        mask = 0x0fff;
    else
        mask = 0x0777;

    initialise_ste_palette(mask);

    if (FALSE) {
        /* Dummy case for conditional compilation */
    }
#if CONF_WITH_VIDEL
    else if (has_videl)
        initialise_falcon_palette(mode);
#endif
#if CONF_WITH_TT_SHIFTER
    else if (has_tt_shifter)
        initialise_tt_palette(rez);
#endif

    fixup_ste_palette(rez);
#endif /* CONF_WITH_ATARI_VIDEO */
}

static char rez_was_hacked;


/* Initialize the video mode and palette. The video memory address will be done later. */
void screen_init_mode(void)
{
#if CONF_WITH_ATARI_VIDEO
    screen_atari_init_mode();
#endif /* CONF_WITH_ATARI_VIDEO */

#ifdef MACHINE_AMIGA
    amiga_screen_init();
#endif

#ifdef MACHINE_LISA
    lisa_screen_init();
#endif

#if defined(MACHINE_A2560U) || defined(MACHINE_A2560K) || defined(MACHINE_A2560M) || defined(MACHINE_A2560X) || defined(MACHINE_GENX)
    a2560_bios_screen_init();
#endif

    rez_was_hacked = FALSE; /* initial assumption */
}


/* Initialize the video address (mode is already set) */
void screen_init_address(void)
{
    LONG vram_size;
    UBYTE *screen_start;
    MAYBE_UNUSED(vram_size);

#if CONF_VRAM_ADDRESS
    vram_size = 0L;         /* unspecified */
    screen_start = (UBYTE *)CONF_VRAM_ADDRESS;
#else
    vram_size = calc_vram_size();
    /* videoram is placed just below the phystop */
    screen_start = balloc_stram(vram_size, TRUE);
#endif /* CONF_VRAM_ADDRESS */

#if CONF_WITH_VIDEL
    video_ram_size = vram_size;     /* these are used by Srealloc() */
    video_ram_addr = screen_start;
#endif

    /* set new v_bas_ad */
    v_bas_ad = screen_start;
    KDEBUG(("v_bas_ad = %p, vram_size = %lu\n", v_bas_ad, vram_size));

#if defined(MACHINE_A2560U) || defined(MACHINE_A2560K) || defined(MACHINE_A2560M) || defined(MACHINE_A2560X) || defined(MACHINE_GENX)
    /* We use a shadow framebuffer, and have code in place to copy it to the VRAM */
    screen_setphys((const UBYTE *)VRAM_Bank0);
#else
    /* correct physical address */
    screen_setphys(screen_start);
#endif
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
int screen_can_change_resolution(void)
{
    if (rez_was_hacked)
        return FALSE;

#ifdef MACHINE_AMIGA
    return TRUE;
#endif

#if CONF_WITH_VIDEL
    if (has_videl)  /* can't change if real ST monochrome monitor */
        return (VgetMonitor() != MON_MONO);
#endif

#if CONF_WITH_ATARI_VIDEO
    return shifter_screen_can_change_resolution();
#else
    return FALSE;
#endif
}


/* get monitor type (same encoding as VgetMonitor()) */
WORD get_monitor_type(void)
{
#if CONF_WITH_ATARI_VIDEO
#if CONF_WITH_VIDEL
    if (has_videl)
        return vmontype();
#endif
    return shifter_get_monitor_type();
#elif defined(MACHINE_A2560U) || defined(MACHINE_A2560K) || defined(MACHINE_A2560M) || defined(MACHINE_A2560X) || defined(MACHINE_GENX)
    return a2560_bios_vmontype();
#else
    return MON_MONO;    /* fake monochrome monitor */
#endif
}


/*
 * calculate VRAM size based on video hardware
 *
 * note: all versions of Atari TOS overallocate memory; we do the same
 * because some programs (e.g. NVDI) rely on this and write past what
 * should be the end of screen memory.
 */
ULONG calc_vram_size(void)
{
#ifdef MACHINE_AMIGA
    return amiga_initial_vram_size();
#elif defined(MACHINE_LISA)
    return 32*1024UL;
#elif defined(MACHINE_A2560U) || defined(MACHINE_A2560K) || defined(MACHINE_A2560M) || defined(MACHINE_A2560X) || defined(MACHINE_GENX)
    return a2560_bios_calc_vram_size();
#else
    return atari_calc_vram_size();
#endif
}


void screen_get_current_mode_info(UWORD *planes, UWORD *hz_rez, UWORD *vt_rez)
{
#ifdef MACHINE_AMIGA
    amiga_get_current_mode_info(planes, hz_rez, vt_rez);
#elif defined(MACHINE_LISA)
    *planes = 1;
    *hz_rez = 720;
    *vt_rez = 364;
#elif defined(MACHINE_A2560U) || defined(MACHINE_A2560K) || defined(MACHINE_A2560M) || defined(MACHINE_A2560X) || defined(MACHINE_GENX)
    a2560_bios_get_current_mode_info(planes, hz_rez, vt_rez);
#else
    atari_get_current_mode_info(planes, hz_rez, vt_rez);
#endif
}

/*
 * used by vdi_v_opnwk()
 *
 * returns the palette (number of colour choices) for the current hardware
 */
WORD get_palette(void)
{
#ifdef MACHINE_AMIGA
    return 2;               /* we currently only support monochrome */
#endif
#if defined(MACHINE_A2560U) || defined(MACHINE_A2560K) || defined(MACHINE_A2560M) || defined(MACHINE_A2560X) || defined(MACHINE_GENX)
    /* All modes are 256 coloursVICKY can do 24bit but this function only returns 16 bits and
     * the VDI, EmuDesk etc. don't support more than 256 colors.
     * So we limit ourselves to 256 as it's done in videl.c */
    return 256;
#else
    return atari_get_palette();
#endif
}


/* returns 'standard' pixel sizes */
static __inline__ void get_std_pixel_size(WORD *width,WORD *height)
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
#if defined(MACHINE_AMIGA) || defined(MACHINE_A2560U) || defined(MACHINE_A2560K) || defined(MACHINE_A2560M) || defined(MACHINE_A2560X) || defined(MACHINE_GENX)
    get_std_pixel_size(width,height);
#else
    if (HAS_VIDEL || HAS_TT_SHIFTER)
        get_std_pixel_size(width,height);
    else
    {
        /* ST TOS has its own set of magic numbers */
        if (5 * V_REZ_HZ >= 12 * V_REZ_VT)  /* includes ST medium */
            *width = 169;
        else if (V_REZ_HZ >= 480)   /* ST high */
            *width = 372;
        else *width = 338;          /* ST low */
        *height = 372;
    }
#endif
}


/* hardware independent xbios routines */

const UBYTE *physbase(void)
{
#ifdef MACHINE_AMIGA
    return amiga_physbase();
#elif defined(MACHINE_LISA)
    return lisa_physbase();
#elif defined(MACHINE_A2560U) || defined(MACHINE_A2560K) || defined(MACHINE_A2560M) || defined(MACHINE_A2560X) || defined(MACHINE_GENX)
    return a2560_bios_physbase();
#elif CONF_WITH_ATARI_VIDEO
    return atari_physbase();
#else
    /* No real physical screen, fall back to Logbase() */
    return logbase();
#endif
}


/* Set physical screen address */

void screen_setphys(const UBYTE *addr)
{
    KDEBUG(("screen_setphys(%p)\n", addr));

#ifdef MACHINE_AMIGA
    amiga_setphys(addr);
#elif defined(MACHINE_LISA)
    lisa_setphys(addr);
#elif defined(MACHINE_A2560U) || defined(MACHINE_A2560K) || defined(MACHINE_A2560M) || defined(MACHINE_A2560X) || defined(MACHINE_GENX)
    a2560_setphys(addr);
#elif CONF_WITH_ATARI_VIDEO
    atari_setphys(addr);
#endif
}


UBYTE *logbase(void)
{
    return v_bas_ad;
}


WORD getrez(void)
{
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
    WORD oldmode = 0;

    if ((LONG)logLoc > 0) {
        v_bas_ad = logLoc;
        KDEBUG(("v_bas_ad = %p\n", v_bas_ad));
    }
    if ((LONG)physLoc > 0) {
        screen_setphys(physLoc);
    }

    /* forbid res changes if Line A variables were 'hacked' or 'rez' is -1 */
    if (rez_was_hacked || (rez == -1)) {
        return 0;
    }

    /* return error for requests for invalid resolutions */
    if ((rez < MIN_REZ) || (rez > MAX_REZ)) {
        return -1;
    }

#if CONF_WITH_VIDEL
    /*
     * if we have videl, and this is a mode change request:
     * 1. fixup videl mode
     * 2. reallocate screen memory & update logical/physical screen addresses
     */
    if (has_videl) {
        if (rez == FALCON_REZ) {
            if (videlmode != -1) {
                videlmode = vfixmode(videlmode);
                if (!logLoc && !physLoc) {
                    UBYTE *addr = (UBYTE *)Srealloc(vgetsize(videlmode));
                    if (!addr)      /* Srealloc() failed */
                        return -1;
                    KDEBUG(("screen realloc'd to %p\n", addr));
                    v_bas_ad = addr;
                    screen_setphys(addr);
                }
            }
            oldmode = vsetmode(-1);
        }
    }
#endif

    /* Wait for the end of display to avoid the plane-shift bug on ST */
    vsync();

#ifdef MACHINE_AMIGA
    amiga_setrez(rez, videlmode);
#elif defined(MACHINE_A2560U) || defined(MACHINE_A2560K) || defined(MACHINE_A2560M) || defined(MACHINE_A2560X) || defined(MACHINE_GENX)
    a2560_bios_setrez(rez, videlmode);
#elif CONF_WITH_ATARI_VIDEO
    atari_setrez(rez, videlmode);
#endif

    screen_init_services_from_mode_info();

    return oldmode;
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

    screen_get_current_mode_info(&planes, &xrez, &yrez);
    screen_init_services(planes, xrez, yrez);
}


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
    /* next VBL will do this */
    colorptr = palettePtr;
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
#ifdef MACHINE_AMIGA
    return amiga_setcolor(colorNum, color);
#elif CONF_WITH_ATARI_VIDEO
    return atari_setcolor(colorNum, color);
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

