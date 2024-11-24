/*
 * screen_atari.c - Atari ST & derivatives-specific XBIOS support and low-level screen routines
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

#include "emutos.h"
#include "asm.h"
#include "bios.h"
#include "biosext.h"
#include "country.h"
#include "dmasound.h"
#include "has.h"
#include "../obj/header.h"
#include "lineavars.h"
#include "mfp.h"
#include "nvram.h"
#include "screen.h"
#include "screen_atari.h"
#include "screen_tt.h"
#include "tosvars.h"
#include "vectors.h"
#include "videl.h"
#include "xbiosbind.h"

#if CONF_WITH_ATARI_VIDEO

static BOOL get_default_palmode(void);


static WORD shifter_check_moderez(WORD moderez);

/*
 * In the original TOS there used to be an early screen init,
 * before memory configuration. This is not used here, and all is
 * done at the same time from C.
 */
void screen_atari_init_mode(void) {
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
 * Initialise ST(e) palette registers
 */
void initialise_ste_palette(UWORD mask)
{
    volatile UWORD *col_regs = (UWORD *) ST_PALETTE_REGS;
    int i;

    for (i = 0; i < 16; i++)
        col_regs[i] = default_palette[i] & mask;
}


WORD atari_setcolor(WORD colorNum, WORD color)
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


void atari_setrez(WORD rez, WORD videlmode)
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


WORD atari_getrez(void)
{
    UBYTE rez;

#if CONF_WITH_VIDEL
    if (has_videl) {
        return videl_getrez();
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


void atari_setphys(const UBYTE *addr)
{
    *(volatile UBYTE *) VIDEOBASE_ADDR_HI = ((ULONG) addr) >> 16;
    *(volatile UBYTE *) VIDEOBASE_ADDR_MID = ((ULONG) addr) >> 8;

    if (HAS_VIDEL || HAS_TT_SHIFTER || HAS_STE_SHIFTER)
        *(volatile UBYTE *) VIDEOBASE_ADDR_LOW = ((ULONG) addr);
}


const UBYTE *atari_physbase(void)
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


ULONG atari_calc_vram_size(void)
{
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


void atari_get_current_mode_info(UWORD *planes, UWORD *hz_rez, UWORD *vt_rez)
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

WORD atari_get_palette(void)
{
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
}


/*
 * Fixup ST(e) palette registers so the highest available color is
 * always "black" (by default)
 */
void fixup_ste_palette(WORD rez)
{
    volatile UWORD *col_regs = (UWORD *) ST_PALETTE_REGS;

    if (rez == ST_MEDIUM)
        col_regs[3] = col_regs[15];
    else if (rez == ST_HIGH)
        col_regs[1] = col_regs[15];
}


/*
 * detect_monitor_change(): called by VBL interrupt handler
 *
 * this checks if the current monitor mode (monochrome/colour) is the
 * same as that set in the shifter.  if not, it calls swv_vec() which
 * by default does a system restart.
 */
void detect_monitor_change(void)
{
    SBYTE monoflag;
    volatile SBYTE *gpip = ((volatile SBYTE *)0xfffffa01L);
    volatile UBYTE *shifter;
    UBYTE monores;
    UBYTE curres;
    UBYTE newres;

    /* not supported on VIDEL */
    if (HAS_VIDEL)
        return;

    monoflag = *gpip;
#if CONF_WITH_DMASOUND
    if (HAS_DMASOUND)
    {
        WORD sr = set_sr(0x2700);
        SBYTE monoflag2;
        SBYTE dmaplay;

        /*
         * on systems with DMA sound, the 'DMA sound active' bit (bit 0
         * of 0xffff8901) is XOR'ed with the monochrome detect bit before
         * being presented at MFP GPIP bit 7.  therefore we must read both
         * bits in order to determine the monitor type.  since the 'sound
         * active' bit can be changed by the hardware at any time, we must
         * avoid a race condition.  the following code waits for both the
         * 'sound active' bit and MFP GPIP bit 7 to stabilise before
         * determining the type of monitor connected.
         */
        for (;;)
        {
            dmaplay = *((volatile SBYTE *)DMASOUND_CTRL);
            monoflag = *gpip;
            monoflag2 = *gpip;
            if ((monoflag ^ monoflag2) < 0)
                continue;
            if (*((volatile SBYTE *)DMASOUND_CTRL) == dmaplay)
                break;
        }

        set_sr(sr);
        if (dmaplay & 1)
            monoflag = -monoflag;
    }
#endif

#if CONF_WITH_TT_SHIFTER
    if (HAS_TT_SHIFTER)
    {
        shifter = ((volatile UBYTE *)TT_SHIFTER);
        curres = *shifter & 7;
        monores = TT_HIGH;
    }
    else
#endif
    /* Assume ST(e) shifter */
    {
        shifter = ((volatile UBYTE *)ST_SHIFTER);
        curres = *shifter & 3;
        monores = ST_HIGH;
    }

    if (curres == monores)  /* current resolution is mono */
    {
        if (monoflag >= 0)  /* mono monitor detected */
            return;
        /* colour monitor detected: switch resolution */
        newres = defshiftmod;   /* use default shifter mode */
        if (newres == monores)  /* but if it's mono, make it ST LOW */
            newres = ST_LOW;
    }
    else        /* current resolution is a colour resolution */
    {
        if (monoflag < 0)   /* & colour monitor detected */
            return;
        /* mono monitor detected: switch resolution */
#if 0
        /*
         * TOS 2.06 & 3.06 (at least) call this here to wait until just
         * after a VBL.  it is surmised that this is because:
         * (a) experience shows that at least some video hardware
         *     misbehaves if the shifter value is not changed 'soon'
         *     after the interrupt, and
         * (b) in TOS 2/3, the vblqueue is processed before this routine
         *     is called, and thus lengthy vblqueue function(s) could
         *     trigger the misbehaviour.
         */
        vsync();
#endif
        newres = monores;
    }

    sshiftmod = newres;
    *shifter = (*shifter & 0xf8) | newres;
    (*swv_vec)();
}


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

int shifter_screen_can_change_resolution(void)
{
    int rez = Getrez();     /* we might be in running in user mode */

    if (HAS_TT_SHIFTER)
        return (rez != TT_HIGH);

    return (rez != ST_HIGH);    /* can't change if mono monitor */
}

WORD shifter_get_monitor_type(void)
{
    volatile UBYTE *gpip = (UBYTE *)0xfffffa01;

    if (*gpip & 0x80)
        return MON_COLOR;
    else
        return MON_MONO;
}


WORD atari_check_moderez(WORD moderez)
{
#if CONF_WITH_VIDEL
    if (has_videl)
        return videl_check_moderez(moderez);
#endif
    return shifter_check_moderez(moderez);
}
#endif
