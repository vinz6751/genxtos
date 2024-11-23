/*
 * screen.c - low-level screen routines
 *
 * Copyright (C) 2001-2024 The EmuTOS development team
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
#include "machine.h"
#include "has.h"
#include "screen.h"
#include "videl.h"
#include "asm.h"
#include "tosvars.h"
#include "linea.h" // linea_set_screen_shift
#include "lineavars.h"
#include "mfp.h"
#include "nvram.h"
#include "font.h"
#include "vt52.h"
#include "xbiosbind.h"
#include "vectors.h"
#include "country.h"
#include "../obj/header.h"
#include "biosmem.h"
#include "biosext.h"
#include "bios.h"
#include "bdosbind.h"
#include "amiga.h"
#include "lisa.h"
#include "nova.h"
#include "screen_atari.h"
#include "screen_tt.h"
#include "a2560u_bios.h"


#if CONF_WITH_VIDEL
LONG video_ram_size;        /* these are used by Srealloc() */
void *video_ram_addr;
#endif

#if CONF_WITH_ATARI_VIDEO

/* 16 color palette 0x0RGB format (4 bits per component) */

const UWORD default_palette[] = {
    RGB_WHITE, RGB_RED, RGB_GREEN, RGB_YELLOW,
    RGB_BLUE, RGB_MAGENTA, RGB_CYAN, RGB_LTGRAY,
    RGB_GRAY, RGB_LTRED, RGB_LTGREEN, RGB_LTYELLOW,
    RGB_LTBLUE, RGB_LTMAGENTA, RGB_LTCYAN, RGB_BLACK
};


static WORD shifter_check_moderez(WORD moderez)
{
    WORD return_rez;

    /*
     * videl modes are not valid for the shifter, so we return zero
     * to indicate that the resolution should not be changed
     */
    if (moderez > 0)                        /* ignore mode values */
        return 0;

    return_rez = moderez & 0x00ff;

    if (HAS_TT_SHIFTER) {
        if (return_rez == TT_HIGH)
            return_rez = TT_MEDIUM;
    } else {
        if (return_rez == ST_HIGH)
            return_rez = ST_LOW;
    }

    return (return_rez==getrez())?0:(0xff00|return_rez);
}

static int shifter_screen_can_change_resolution(void)
{
    int rez = Getrez();     /* we might be in running in user mode */

    if (HAS_TT_SHIFTER)
        return (rez != TT_HIGH);

    return (rez != ST_HIGH);    /* can't change if mono monitor */
}

static WORD shifter_get_monitor_type(void)
{
    volatile UBYTE *gpip = (UBYTE *)0xfffffa01;

    if (*gpip & 0x80)
        return MON_COLOR;
    else
        return MON_MONO;
}

#endif /* CONF_WITH_ATARI_VIDEO */


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

#if CONF_WITH_VIDEL
    if (has_videl)
        return videl_check_moderez(moderez);
#endif

#if CONF_WITH_ATARI_VIDEO
    return shifter_check_moderez(moderez);
#else
    return 0;
#endif
}

/*
 * Initialise palette registers
 * This routine is also used by resolution change
 */
void initialise_palette_registers(WORD rez,WORD mode)
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

/* Get the default PAL/NTSC mode according to OS header.
 * Returns TRUE for PAL 50 Hz, or FALSE for NTSC 60 Hz.
 */
static BOOL get_default_palmode(void)
{
    if (os_header.os_conf == OS_CONF_MULTILANG)
    {
        /* No country/mode specified in OS header.
         * The mode is inferred from the COUNTRY Makefile variable. */
        return OS_PAL;
    }
    else
    {
        /* Use the mode specified in OS header */
        return os_header.os_conf & 0x0001;
    }
}

/*
 * In the original TOS there used to be an early screen init,
 * before memory configuration. This is not used here, and all is
 * done at the same time from C.
 */

/* Initialize the video mode (address will be done later) */
void screen_init_mode(void)
{
#if CONF_WITH_ATARI_VIDEO
#if CONF_WITH_VIDEL
    UWORD boot_resolution = FALCON_DEFAULT_BOOT;
#endif
    WORD monitor_type, sync_mode;
    WORD rez = 0;   /* avoid 'may be uninitialized' warning */

    /* Initialize the interrupt handlers & the VBL semaphore.
     * It is important to do this first because the initialization code below
     * may call vsync(), which temporarily enables the interrupts. */
    setexc((VEC_HBL/4), (LONG)int_hbl);
    setexc((VEC_VBL/4), (LONG)int_vbl);
    vblsem = 0;

    /* first, see what we're connected to, and set the
     * resolution / video mode appropriately */
    monitor_type = get_monitor_type();
    KDEBUG(("monitor_type = %d\n", monitor_type));

    /* Then figure out and set the default video mode */

#if CONF_WITH_VIDEL
    if (has_videl) {
        WORD ret;

        MAYBE_UNUSED(ret);

        /* reset VIDEL on boot-up */
        /* first set the physbase to a safe memory */
#if CONF_VRAM_ADDRESS
        screen_setphys((const UBYTE *)CONF_VRAM_ADDRESS);
#else
        screen_setphys((const UBYTE *)0x10000L);
#endif

#if CONF_WITH_NVRAM && !defined(MACHINE_FIREBEE)
        /* This is currently disabled on the FireBee, because the VIDEL is
         * unreliable. Some video modes are not displayed well.
         */
        /* get boot resolution from NVRAM */
        ret = nvmaccess(0, 14, 2, CONST_CAST(UBYTE*, &boot_resolution));
        if (ret != 0) {
            KDEBUG(("Invalid NVRAM, defaulting to boot video mode 0x%04x\n", boot_resolution));
        }
        else {
            KDEBUG(("NVRAM boot video mode is 0x%04x\n", boot_resolution));
        }
#endif /* CONF_WITH_NVRAM */

        /* try to ensure it corresponds to monitor */
        current_video_mode = boot_resolution;       /* needed by vfixmode() */
        boot_resolution = vfixmode(boot_resolution);
        if (!lookup_videl_mode(boot_resolution)) {  /* mode isn't in table */
            KDEBUG(("Invalid video mode 0x%04x changed to 0x%04x\n",
                    boot_resolution,FALCON_DEFAULT_BOOT));
            boot_resolution = FALCON_DEFAULT_BOOT;  /* so pick one that is */
        }

        if (!VALID_VDI_BPP(boot_resolution)) {      /* mustn't confuse VDI */
            KDEBUG(("VDI doesn't support video mode 0x%04x, changed to 0x%04x\n",
                    boot_resolution,FALCON_DEFAULT_BOOT));
            boot_resolution = FALCON_DEFAULT_BOOT;  /* so use default */
        }

        /* vsetmode() now uses vfixmode() to adjust the video mode
         * according to the actual monitor
         */
        vsetmode(boot_resolution);  /* sets 'sshiftmod' */
        rez = sshiftmod;
        KDEBUG(("Fixed boot video mode is 0x%04x\n",vsetmode(-1)));
    }
    else
#endif /* CONF_WITH_VIDEL */

#if CONF_WITH_TT_SHIFTER
    if (has_tt_shifter) {
        sshiftmod = rez = monitor_type ? TT_MEDIUM : TT_HIGH;
        *(volatile UBYTE *) TT_SHIFTER = rez;
    }
    else
#endif
    {
        /* On ST, it is important to change the resolution register when nothing
         * is displaying, otherwise the plane shift bug may appear. */
        vsync();

#if CONF_WITH_RESET
        /* If the Glue was reset during startup, it will sometimes need a second
         * VSYNC interrupt to settle again.  Without this second wait for VSYNC,
         * a monochrome screen display may wrap, or black bars may appear. */
        vsync();
#endif

        sshiftmod = rez = monitor_type ? ST_LOW : ST_HIGH;
        *(volatile UBYTE *) ST_SHIFTER = rez;

#if CONF_WITH_STE_SHIFTER
        /* On the STe, reset the additional video registers to default values. */
        if (has_ste_shifter) {
            *(volatile UBYTE *)STE_LINE_OFFSET = 0;
            *(volatile UBYTE *)STE_HORZ_SCROLL = 0;
        }
#endif
    }

    // Then adjust the PAL/NTSC mode
#if CONF_WITH_VIDEL
    if (has_videl) {        /* detected a Falcon */
        sync_mode = (boot_resolution&VIDEL_PAL)?0x02:0x00;
    }
    else
#endif
    if (HAS_TT_SHIFTER)
    {
        /* In the "Atari TT030 Hardware Reference Manual" (June 1990),
         * bit 0 of the ST Sync Mode register is noted as 'set to 1'.
         */
        sync_mode = 0x01;
    }
    else
    {
        BOOL palmode = get_default_palmode();
        sync_mode = palmode ? 0x02 : 0x00;
    }
    *(volatile UBYTE *) SYNCMODE = sync_mode;

/*
 * next, set up the palette(s)
 */
#if CONF_WITH_VIDEL
    initialise_palette_registers(rez,boot_resolution);
#else
    initialise_palette_registers(rez,0);
#endif

#endif /* CONF_WITH_ATARI_VIDEO */
    MAYBE_UNUSED(get_default_palmode);

#ifdef MACHINE_AMIGA
    amiga_screen_init();
#endif

#ifdef MACHINE_LISA
    lisa_screen_init();
#endif

#if defined(MACHINE_A2560U) || defined(MACHINE_A2560X)
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

#if defined(MACHINE_A2560U) || defined(MACHINE_A2560X)
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

    screen_init_services();
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
#if CONF_WITH_VIDEL
    if (has_videl)
        return vmontype();
#endif

#if CONF_WITH_ATARI_VIDEO
    return shifter_get_monitor_type();
#elif defined(MACHINE_A2560U) || defined(MACHINE_A2560X)
    return a2560_bios_vmontype();
#else
    return MON_MONO;    /* fake monochrome monitor */
#endif
}

/* Settings for the different video modes */
struct video_mode {
    UWORD       planes;         /* count of color planes (v_planes) */
    UWORD       hz_rez;         /* screen horizontal resolution (v_hz_rez) */
    UWORD       vt_rez;         /* screen vertical resolution (v_vt_rez) */
};

static const struct video_mode const vmode_table[] = {
    { 4,  320, 200},            /* rez=0: ST low */
    { 2,  640, 200},            /* rez=1: ST medium */
    { 1,  640, 400},            /* rez=2: ST high */
#if CONF_WITH_TT_SHIFTER
    { 0,    0,   0},            /* rez=3: invalid */
    { 4,  640, 480},            /* rez=4: TT medium */
    { 0,    0,  0,},            /* rez=5: invalid */
    { 1, 1280, 960},            /* rez=6: TT high */
    { 8,  320, 480},            /* rez=7: TT low */
#endif
};

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
#elif defined(MACHINE_A2560U) || defined(MACHINE_A2560X)
    return a2560_bios_calc_vram_size();
#else
    ULONG vram_size;

    if (HAS_VIDEL)
        return FALCON_VRAM_SIZE + EXTRA_VRAM_SIZE;

    vram_size = (ULONG)BYTES_LIN * V_REZ_VT;

    /* TT TOS allocates 256 bytes more than actually needed. */
    if (HAS_TT_SHIFTER)
        return vram_size + EXTRA_VRAM_SIZE;

    /*
     * The most important issue for the ST is ensuring that screen memory
     * starts on a 256-byte boundary for hardware reasons.  We assume
     * that screen memory is allocated at the top of memory, and that
     * memory ends on a 256-byte boundary.  So we must allocate a multiple
     * of 256 bytes.  For compatibility with ST TOS, we also allocate
     * (at least) 768 bytes more than actually needed.
     */
    return (vram_size + 768UL + 255UL) & ~255UL;
#endif
}

static void shifter_get_current_mode_info(UWORD *planes, UWORD *hz_rez, UWORD *vt_rez)
{
    WORD vmode;                         /* video mode */

    vmode = (sshiftmod & 7);            /* Get video mode from copy of hardware */
    KDEBUG(("vmode: %d\n", vmode));

    *planes = vmode_table[vmode].planes;
    *hz_rez = vmode_table[vmode].hz_rez;
    *vt_rez = vmode_table[vmode].vt_rez;
}

static void atari_get_current_mode_info(UWORD *planes, UWORD *hz_rez, UWORD *vt_rez)
{
#if CONF_WITH_VIDEL
    if (has_videl) {
        videl_get_current_mode_info(planes, hz_rez, vt_rez);
    } else
#endif
    {
        shifter_get_current_mode_info(planes, hz_rez, vt_rez);
    }
}

void screen_get_current_mode_info(UWORD *planes, UWORD *hz_rez, UWORD *vt_rez)
{
    MAYBE_UNUSED(atari_get_current_mode_info);

#ifdef MACHINE_AMIGA
    amiga_get_current_mode_info(planes, hz_rez, vt_rez);
#elif defined(MACHINE_LISA)
    *planes = 1;
    *hz_rez = 720;
    *vt_rez = 364;
#elif defined(MACHINE_A2560U) || defined(MACHINE_A2560X)
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
#if defined(MACHINE_A2560U) || defined(MACHINE_A2560X)
    /* All modes are 256 coloursVICKY can do 24bit but this function only returns 16 bits and
     * the VDI, EmuDesk etc. don't support more than 256 colors.
     * So we limit ourselves to 256 as it's done in videl.c */
    return 256;
#else
    WORD palette;

#if CONF_WITH_VIDEL
    /* we return the same values as Atari TOS 4.04 */
    if (has_videl)
    {
        WORD mode = vsetmode(-1);
        if ((mode&VIDEL_COMPAT)
         || ((mode&VIDEL_BPPMASK) == VIDEL_4BPP))
            return 4096;
        return 0;
    }
#endif /* CONF_WITH_VIDEL */

    palette = 4096;         /* for STe/TT colour modes */

    switch(sshiftmod) {
    case ST_HIGH:
#if CONF_WITH_TT_SHIFTER
    case TT_HIGH:
#endif
        return 2;
    case ST_LOW:
    case ST_MEDIUM:
#if CONF_WITH_STE_SHIFTER
        if (has_ste_shifter)
            break;
#endif
        palette = 512;     /* colour modes on plain ST */
    }

    return palette;
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
#if defined(MACHINE_AMIGA) || defined(MACHINE_A2560U) || defined(MACHINE_A2560X)
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

#if CONF_WITH_ATARI_VIDEO

static const UBYTE *atari_physbase(void)
{
    ULONG addr;

#if CONF_WITH_NOVA
    if (HAS_NOVA && rez_was_hacked) {
        /* Nova/Vofa present and in use? Return its screen memory */
        return get_novamembase();
    }
#endif

    addr = *(volatile UBYTE *) VIDEOBASE_ADDR_HI;
    addr <<= 8;
    addr |= *(volatile UBYTE *) VIDEOBASE_ADDR_MID;
    addr <<= 8;

    if (HAS_VIDEL || HAS_TT_SHIFTER || HAS_STE_SHIFTER)
        addr |= *(volatile UBYTE *) VIDEOBASE_ADDR_LOW;

    return (UBYTE *)addr;
}

static void atari_setphys(const UBYTE *addr)
{
    *(volatile UBYTE *) VIDEOBASE_ADDR_HI = ((ULONG) addr) >> 16;
    *(volatile UBYTE *) VIDEOBASE_ADDR_MID = ((ULONG) addr) >> 8;

    if (HAS_VIDEL || HAS_TT_SHIFTER || HAS_STE_SHIFTER)
        *(volatile UBYTE *) VIDEOBASE_ADDR_LOW = ((ULONG) addr);
}

static WORD atari_getrez(void)
{
    UBYTE rez;

#if CONF_WITH_VIDEL
    if (has_videl) {
        /* Get the video mode for Falcon-hardware */
        WORD vmode = vsetmode(-1);
        if (vmode & VIDEL_COMPAT) {
            switch(vmode&VIDEL_BPPMASK) {
            case VIDEL_1BPP:
                rez = 2;
                break;
            case VIDEL_2BPP:
                rez = 1;
                break;
            case VIDEL_4BPP:
                rez = 0;
                break;
            default:
                KINFO(("Problem - unsupported video mode\n"));
                rez = 0;
            }
        } else
            rez = 2;
    }
    else
#endif
#if CONF_WITH_TT_SHIFTER
    if (has_tt_shifter) {
        /* Get the video mode for TT-hardware */
        rez = *(volatile UBYTE *)TT_SHIFTER & 0x07;
    }
    else
#endif
    {
        rez = *(volatile UBYTE *)ST_SHIFTER & 0x03;
    }

    return rez;
}

static void atari_setrez(WORD rez, WORD videlmode)
{
    if (FALSE) {
        /* Dummy case for conditional compilation */
    }
#if CONF_WITH_VIDEL
    else if (has_videl) {
        if ((rez >= ST_LOW) && (rez <= FALCON_REZ)) {
            videl_setrez(rez, videlmode);   /* sets 'sshiftmod' */
            /* Atari TOS 4 re-inits the palette */
            initialise_falcon_palette(videlmode);
        }
    }
#endif
#if CONF_WITH_TT_SHIFTER
    else if (has_tt_shifter) {
        if ((rez != FALCON_REZ) && (rez != REZ_UNSUPPORTED))
            *(volatile UBYTE *)TT_SHIFTER = sshiftmod = rez;
    }
#endif
    else if (rez <= ST_HIGH) {         /* ST resolution */
        *(volatile UBYTE *)ST_SHIFTER = sshiftmod = rez;
    }
}

static WORD atari_setcolor(WORD colorNum, WORD color)
{
    WORD oldcolor;

    WORD mask;
    volatile WORD *palette = (WORD *) ST_PALETTE_REGS;

    KDEBUG(("Setcolor(0x%04x, 0x%04x)\n", colorNum, color));

    colorNum &= 0x000f;         /* just like real TOS */

    if (HAS_VIDEL || HAS_TT_SHIFTER || HAS_STE_SHIFTER)
        mask = 0x0fff;
    else
        mask = 0x0777;

    oldcolor = palette[colorNum] & mask;
    if (color >= 0)
        palette[colorNum] = color;  /* update ST(e)-compatible palette */

    return oldcolor;
}

#endif /* CONF_WITH_ATARI_VIDEO */

/* hardware independent xbios routines */

const UBYTE *physbase(void)
{
#ifdef MACHINE_AMIGA
    return amiga_physbase();
#elif defined(MACHINE_LISA)
    return lisa_physbase();
#elif defined(MACHINE_A2560U) || defined(MACHINE_A2560X)
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
#elif defined(MACHINE_A2560U) || defined(MACHINE_A2560X)
    a2560u_setphys(addr);
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
#elif defined(MACHINE_A2560U) || defined(MACHINE_A2560X)
    a2560_bios_setrez(rez, videlmode);
#elif CONF_WITH_ATARI_VIDEO
    atari_setrez(rez, videlmode);
#endif

    screen_init_services();

    return oldmode;
}

void screen_init_services(void)
{
    KDEBUG(("screen_init_services\n"));
    /* Temporarily halt VBL processing */
    vblsem = 0;
    /* Re-initialize line-a, VT52 etc: */
    linea_init();
    vt52_init();
    /* Restart VBL processing */
    vblsem = 1;
}

void setpalette(const UWORD *palettePtr)
{
#ifdef ENABLE_KDEBUG
    int i, max;
    max = getrez() == 0 ? 15 : getrez() == 1 ? 3 : 1;
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

