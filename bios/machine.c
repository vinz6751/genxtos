/*
 * machine.c - detection of machine type
 *
 * Copyright (C) 2001-2025 The EmuTOS development team
 *
 * Authors:
 *  LVL     Laurent Vogel
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#define ENABLE_KDEBUG

#include "emutos.h"
#include "bios.h" /* boot_status */
#include "cookie.h"
#include "machine.h"
#include "has.h"
#include "processor.h"
#include "biosmem.h"
#include "vectors.h"
#include "nvram.h"
#include "tosvars.h"
#include "country.h"
#include "clock.h"
#include "natfeat.h"
#include "xhdi.h"
#include "screen_tt.h"
#include "string.h"
#include "dmasound.h"
#include "dsp.h"
#include "acsi.h"
#include "scsi.h"
#include "ide.h"
#include "scsidriv.h"
#include "asm.h"
#include "delay.h"
#include "mfp.h"
#include "scc.h"
#include "memory.h"
#include "coldfire.h"
#include "dma.h"
#include "nova.h"
#include "biosext.h"
#include "timer.h"
#include "amiga.h"
#include "a2560u_bios.h"

#if CONF_WITH_ADVANCED_CPU
UBYTE is_bus32; /* 1 if address bus is 32-bit, 0 if it is 24-bit */
#endif

ULONG detected_busses;  /* bitmap of i/o busses detected */

ULONG cookie_vdo;
#if CONF_WITH_FDC
ULONG cookie_fdc;
#endif
ULONG cookie_snd;
ULONG cookie_mch;
#if CONF_WITH_DIP_SWITCHES
ULONG cookie_swi;
#endif


/*
 * test specific hardware features
 */
#if CONF_WITH_STE_SHIFTER
int has_ste_shifter;
#endif
#if CONF_WITH_TT_SHIFTER
int has_tt_shifter;
#endif
#if CONF_WITH_VIDEL
int has_videl;
#endif

#if CONF_ATARI_HARDWARE

int has_modectl;

/*
 * Check if the DMA 'modectl' register exists
 *
 * A bus error _may_ occur when accessing 'modectl' on some systems that
 * do not support HD floppies [Christian Zietz's investigations indicate
 * that the error occurs on systems with the IMP chip set].
 */
static void detect_modectl(void)
{
    has_modectl = 0;
    if (check_read_byte((LONG)&DMA->modectl))
        has_modectl = 1;

    KDEBUG(("has_modectl = %d\n", has_modectl));
}

#endif

/*
 * Tests video capabilities (STe Enhanced Shifter, TT Shifter and VIDEL)
 */
static void detect_video(void)
{
#if CONF_WITH_STE_SHIFTER
    /* test if we have an STe Shifter by testing if register 820d
     * works (put a value, read other reg, read again, and compare)
     */
    volatile UBYTE *ste_reg = (UBYTE *) 0xffff820d;
    volatile UBYTE *other_reg1 = (UBYTE *) 0xffff8203;
    volatile WORD *other_reg2 = (WORD *) 0xffff8240;

    has_ste_shifter = 0;
    if (!check_read_byte((long)ste_reg))
        return;
    *ste_reg = 90;
    *other_reg1;        /* force register read (really useful ?) */
    if (*ste_reg == 90)
    {
        *ste_reg = 0;
        *other_reg2;    /* force register read (really useful ?) */
        if (*ste_reg == 0)
            has_ste_shifter = 1;
    }

    KDEBUG(("has_ste_shifter = %d\n", has_ste_shifter));
#endif

#if CONF_WITH_TT_SHIFTER
    /* test if we have a TT Shifter by testing for TT color palette */
    has_tt_shifter = 0;
    if (check_read_byte((long)TT_PALETTE_REGS))
        has_tt_shifter = 1;

    KDEBUG(("has_tt_shifter = %d\n", has_tt_shifter));
#endif

#if CONF_WITH_VIDEL
    /* test if we have Falcon VIDEL by testing for f030_xreg */
    has_videl = 0;
    if (check_read_byte(FALCON_HHT))
        has_videl = 1;

    KDEBUG(("has_videl = %d\n", has_videl));

    /*
     * The Falcon Bus Control Register uses the following bits:
     *   0x40 : type of start (0=cold, 1=warm)
     *   0x20 : STe Bus emulation (0=on, 1=off)
     *   0x08 : blitter control (0=on, 1=off)
     *   0x04 : blitter speed (0=8MHz, 1=16MHz)
     *   0x01 : cpu speed (0=8MHz, 1=16MHz)
     * Source: Hatari source code
     *
     * STe Bus emulation needs to be switched off for
     * bus-error-based hardware detection to work on the Falcon.
     */
    if (has_videl)      /* i.e. it's a Falcon */
    {
        volatile UBYTE *fbcr = (UBYTE *)FALCON_BUS_CTL;
        *fbcr |= 0x25;  /* set STe Bus emulation off, 16MHz blitter & CPU */
    }
#endif
}

#if CONF_WITH_TT_MFP

int has_tt_mfp;

/*
 * detect second MFP (TT only)
 */
static void detect_tt_mfp(void)
{
    has_tt_mfp = 0;
    if (check_read_byte((LONG)TT_MFP_BASE+1))
        has_tt_mfp = 1;

    KDEBUG(("has_tt_mfp = %d\n", has_tt_mfp));
}

#endif /* CONF_WITH_TT_MFP */

#if CONF_WITH_SCC

int has_scc;

/*
 * detect SCC (Falcon and TT)
 */
static void detect_scc(void)
{
    has_scc = 0;
    if (check_read_byte(SCC_BASE))
        has_scc = 1;

    KDEBUG(("has_scc = %d\n", has_scc));
}

#endif /* CONF_WITH_SCC */

#if CONF_WITH_VME

int has_vme;

static void detect_vme(void)
{
    volatile UBYTE *vme_mask = (UBYTE *) VME_INT_MASK;
    volatile UBYTE *sys_mask = (UBYTE *) SYS_INT_MASK;

    if (check_read_byte(SCU_GPR1))
    {
        *vme_mask = VME_INT_MFP;                    /* allow MFP interrupts */
        *sys_mask = SYS_INT_VSYNC | SYS_INT_HSYNC;  /* allow VSYNC, HSYNC */
        has_vme = 1;
    } else {
        has_vme = 0;
    }

    KDEBUG(("has_vme = %d\n", has_vme));
}

#endif /* CONF_WITH_VME */

#if CONF_WITH_MONSTER

int has_monster;

static void detect_monster(void)
{
    if (IS_ARANYM)
    {
        /* The auto-detection currently crashes ARAnyM-JIT. */
        has_monster = 0;
    }
    else
    {
        has_monster = check_read_byte(MONSTER_REG);
    }

    KDEBUG(("has_monster = %d\n", has_monster));
}

#endif /* CONF_WITH_MONSTER */

#if CONF_WITH_MAGNUM

int has_magnum;

/*
 * These are the magic addresses for the activation sequence used by the
 * Magnum driver. Actually, the least significant 16 bits are 'don't care'
 * due to the hardware design.
 */
#define MAGNUM_MAGIC1    (0x6C4710ul)
#define MAGNUM_MAGIC2    (0x5D1234ul)
#define MAGNUM_MAGIC3    (0x6D3148ul)

#define MAGNUM_MEM_START (0x400000ul)

static void detect_magnum(void)
{
    /* Assume no Magnum RAM expansion card. */
    has_magnum = 0;

    do
    {
        /* ARAnyM cannot have a Magnum. */
        if (IS_ARANYM)
            break;
        /*
         * Magnum will be initially disabled. Thus check that there *is*
         * a bus error at 4 MB. Otherwise there already is RAM or another
         * peripheral at that address and not a Magnum.
         */
        if (check_read_byte(MAGNUM_MEM_START))
            break;
        /*
         * Magnum is enabled by a sequence of reads to magic addresses.
         * These reads cause a bus-error on the Magnum. If they don't,
         * it's not a Magnum card.
         */
        if (check_read_byte(MAGNUM_MAGIC1))
            break;
        if (check_read_byte(MAGNUM_MAGIC2))
            break;
        if (check_read_byte(MAGNUM_MAGIC3))
            break;
        /* If Magnum is present, an access to 4 MB must now work. */
        has_magnum = check_read_byte(MAGNUM_MEM_START);
    } while (0);

    KDEBUG(("has_magnum = %d\n", has_magnum));
}

#endif /* CONF_WITH_MAGNUM */

#if CONF_WITH_BLITTER

/* blitter */

int has_blitter;
int blitter_is_enabled;

static void detect_blitter(void)
{
    has_blitter = blitter_is_enabled = 0;

    /*
     * although no Atari-developed system has both TT-RAM and a blitter,
     * some add-ons to Atari systems do (e.g. a CT60 in 68060 mode).
     * because the Atari blitter only supports 24-bit addressing, we must
     * prevent its use on such systems.  we do allow a blitter on the
     * FireBee, which by design supports 32-bit blitting.
     */
#ifndef MACHINE_FIREBEE
    if (!ramtop)
#endif
        if (check_read_byte(BLITTER_CONFIG1))
            has_blitter = 1;

    KDEBUG(("has_blitter = %d\n", has_blitter));
}

#endif /* CONF_WITH_BLITTER */


#if CONF_WITH_DIP_SWITCHES

int has_dip_switches;

static void detect_dip_switches(void)
{
    if (IS_ARANYM)
    {
        /* The auto-detection currently crashes ARAnyM-JIT. */
        has_dip_switches = 0;
    }
    else
    {
        has_dip_switches = check_read_byte(DIP_SWITCHES+1);
    }

    KDEBUG(("has_dip_switches = %d\n", has_dip_switches));
}

/* DIP switch usage is as follows (according to the "ATARI FALCON030
 * Service Guide", dated October 1, 1992):
 * bit 7: off => no DMA sound hardware
 * bit 6: off => AJAX FDC chip installed (support for 1.44MB floppy)
 * bit 5: off => quad density floppy
 * other bits are not used (and are set on)
 */

static void setvalue_swi(void)
{
    cookie_swi = HIBYTE(*(volatile UWORD *)DIP_SWITCHES);
    KDEBUG(("cookie_swi = 0x%08lx\n", cookie_swi));
}

#endif /* CONF_WITH_DIP_SWITCHES */

/* video type */

static void setvalue_vdo(void)
{
#if CONF_ATARI_HARDWARE
    if (HAS_VIDEL)
        cookie_vdo = VDO_FALCON;
    else if (HAS_TT_SHIFTER)
        cookie_vdo = VDO_TT;
    else if (HAS_STE_SHIFTER)
        cookie_vdo = VDO_STE;
    else
        cookie_vdo = VDO_ST;
#elif defined(MACHINE_A2560U)
    cookie_vdo = MCH_A2560U;
#elif defined(MACHINE_A2560X)
    cookie_vdo = MCH_A2560X;
#else
    cookie_vdo = VDO_NOHARD;
#endif /* CONF_ATARI_HARDWARE */

    KDEBUG(("cookie_vdo = 0x%08lx\n", cookie_vdo));
}

/* machine type */
static void setvalue_mch(void)
{
#if CONF_ATARI_HARDWARE
    if (IS_ARANYM)
        cookie_mch = MCH_ARANYM;
    else if (HAS_VIDEL)
        cookie_mch = MCH_FALCON;
    else if (HAS_TT_SHIFTER)
        cookie_mch = MCH_TT;
    else if (HAS_STE_SHIFTER)
    {
        if (HAS_VME)
            cookie_mch = MCH_MSTE;
        else
            cookie_mch = MCH_STE;
    }
    else
        cookie_mch = MCH_ST;
#elif defined(MACHINE_A2560U)
    cookie_mch = MCH_A2560U;
#elif defined(MACHINE_A2560X)
    cookie_mch = MCH_A2560X;
#else
    cookie_mch = MCH_NOHARD;
#endif /* CONF_ATARI_HARDWARE */

    KDEBUG(("cookie_mch = 0x%08lx\n", cookie_mch));
}

/* SND */

static void setvalue_snd(void)
{
    cookie_snd = 0;

#if CONF_WITH_YM2149
    cookie_snd |= SND_PSG;
#endif

    if (HAS_DMASOUND)
    {
        cookie_snd |= SND_8BIT;
    }

    if (HAS_FALCON_DMASOUND)
    {
        cookie_snd |= SND_16BIT | SND_MATRIX;
    }

    if (HAS_DSP)
    {
        cookie_snd |= SND_DSP;
    }

#if CONF_WITH_DIP_SWITCHES
    if (has_dip_switches)
    {
        /*
         * if DIP sw 8 is on (i.e. bit 7 is off), then we turn off the
         * indicator for 8-bit DMA stereo in the _SND cookie, just like
         * TOS3/TOS4 do.
         */
        if (!(cookie_swi & 0x80))
        {
            cookie_snd &= ~SND_8BIT;
        }
    }
#endif

    KDEBUG(("cookie_snd = 0x%08lx\n", cookie_snd));
}

#if CONF_WITH_FRB

/*
 * FRB (FastRAM Buffer)
 * This needs to be set up after TT-RAM detection,
 * and before BDOS RAM initialization.
 */

static void add_cookie_frb(void)
{
    BOOL need_frb = FALSE; /* Required only if the system has Alt-RAM */

#if CONF_WITH_TTRAM
    /* We need FRB if TT-RAM is present */
    need_frb = (ramtop != NULL);
#endif

#if CONF_WITH_MONSTER
    need_frb |= has_monster;
#endif

#if CONF_WITH_MAGNUM
    need_frb |= has_magnum;
#endif

    if (need_frb)
    {
        UBYTE *cookie_frb = balloc_stram(FRB_SIZE, FALSE);
        cookie_add(COOKIE_FRB, (ULONG)cookie_frb);
        KDEBUG(("cookie_frb = %p\n", cookie_frb));
    }
}

#endif /* CONF_WITH_FRB */

#if CONF_WITH_FDC

/* FDC */

static void setvalue_fdc(void)
{
#if CONF_WITH_DIP_SWITCHES
    if (has_dip_switches && !(cookie_swi & 0x40))
    {
    /* switch *off* means AJAX controller is installed */
        cookie_fdc = FDC_1ATC;
    } else
#endif
    {
        cookie_fdc = FDC_0ATC;
    }

    KDEBUG(("cookie_fdc = 0x%08lx\n", cookie_fdc));
}

#endif /* CONF_WITH_FDC */

#if CONF_WITH_ARANYM
static const char aranym_name[] = "ARAnyM";
int is_aranym;

static void aranym_machine_detect(void)
{
    char buffer[80];
    long bufsize;

    bufsize = nfGetFullName(buffer, sizeof(buffer)-1);
    is_aranym = bufsize > 0 && !strncasecmp(buffer, aranym_name, strlen(aranym_name));

    KDEBUG(("is_aranym = %d\n", is_aranym));
}
#endif

static ULONG check_busses(void)
{
    ULONG found = 0UL;

#if CONF_WITH_ACSI
    if (detect_acsi())
        found |= (1 << ACSI_BUS);
#endif
#if CONF_WITH_SCSI
    if (detect_scsi())
        found |= (1 << SCSI_BUS);
#endif
#if CONF_WITH_IDE
    if (detect_ide())
        found |= (1 << IDE_BUS);
#endif

    return found;
}

/* Detect optional hardware and fill has_* variables accordingly.
 * Those detection routines must *NOT* rely on cookies,
 * because cookies are initialized later.
 */
void machine_detect(void)
{
#if CONF_WITH_ARANYM
    aranym_machine_detect();
#endif
#ifdef MACHINE_AMIGA
    amiga_machine_detect();
#endif
#if CONF_ATARI_HARDWARE
    detect_modectl();
#endif

    /* Detect TT-RAM and set up ramtop/ramvalid */
    KDEBUG(("ttram_detect()\n"));
    ttram_detect();

    detect_video();
#if CONF_WITH_TT_MFP
    if (!IS_ARANYM)
        detect_tt_mfp();
#endif
#if CONF_WITH_SCC
    if (!IS_ARANYM)
        detect_scc();
#endif
#if CONF_WITH_VME
    if (!IS_ARANYM)
        detect_vme();
#endif
#if CONF_WITH_MEGARTC
    if (!IS_ARANYM)
        detect_megartc();
    KDEBUG(("has_megartc = %d\n", has_megartc));
#endif /* CONF_WITH_MEGARTC */
#if CONF_WITH_ICDRTC
    detect_icdrtc();
    KDEBUG(("has_icdrtc = %d\n", has_icdrtc));
#endif /* CONF_WITH_ICDRTC */
#if CONF_WITH_NVRAM
    detect_nvram();
#endif
#if CONF_WITH_DMASOUND
    detect_dmasound();
#endif
#if CONF_WITH_DSP
    detect_dsp();
#endif
#if CONF_WITH_DIP_SWITCHES
    detect_dip_switches();
#endif
#if CONF_WITH_BLITTER
    detect_blitter();
#endif

    detected_busses = check_busses();
    KDEBUG(("detected_busses = 0x%lx\n", detected_busses));

#if CONF_WITH_MONSTER
    detect_monster();
    if (has_monster)
    {
        detect_monster_rtc();
        KDEBUG(("has_monster_rtc = %d\n", has_monster_rtc));
    }
#endif
#if CONF_WITH_MAGNUM
    detect_magnum();
#endif
#if CONF_WITH_NOVA
    if (!IS_ARANYM)
        detect_nova();
#endif
}

/*
 * perform machine-specific initialisation
 */
void machine_init(void)
{
#if defined(MACHINE_A2560U) || defined(MACHINE_A2560X)
    a2560_bios_init();
    /* There is an early setup of the UART so we can use KDEBUG earlier. */
    //TODOboot_status |= RS232_AVAILABLE;
#endif

    /* Initialize the MFP, this ensures their IRQs are disabled */
#if CONF_WITH_MFP
    KDEBUG(("mfp_init()\n"));
    mfp_init();
#endif
#if CONF_WITH_TT_MFP
    if (has_tt_mfp)
    {
        KDEBUG(("mfptt_init()\n"));
        mfptt_init();
    }
#endif

#if !CONF_WITH_RESET
/*
 * we must disable interrupts here, because the reset instruction hasn't
 * been run during startup
 */
 #if CONF_WITH_SCC
    if (has_scc)
    {
        SCC *scc = (SCC *)SCC_BASE;
        ULONG loops = loopcount_1_msec / 1000;  /* 1 usec = 8 cycles of SCC PCLK */

        scc->portA.ctl = 0x09;  /* issue hardware reset */
        delay_loop(loops);
        scc->portA.ctl = 0xC0;
        delay_loop(loops);
    }
 #endif
#endif /* CONF_WITH_RESET */
}

void fill_cookie_jar(void)
{
#ifdef __mcoldfire__
    cookie_add(COOKIE_COLDFIRE, 0);
    setvalue_mcf();
    cookie_add(COOKIE_MCF, (ULONG)&cookie_mcf);
#else
    /* this is detected by detect_cpu(), called from processor_init() */
    cookie_add(COOKIE_CPU, mcpu);
#endif

    /* _VDO
     * This cookie represents the revision of the video shifter present.
     * Currently valid values are:
     * 0x00000000  ST
     * 0x00010000  STe
     * 0x00020000  TT030
     * 0x00030000  Falcon030
     */
    setvalue_vdo();
    cookie_add(COOKIE_VDO, cookie_vdo);

#ifndef __mcoldfire__
  /* this is detected by detect_fpu(), called from processor_init() */
    cookie_add(COOKIE_FPU, fputype);
#endif

    /* _MCH */
    setvalue_mch();
    cookie_add(COOKIE_MCH, cookie_mch);

#if CONF_WITH_DIP_SWITCHES
    /* _SWI  On machines that contain internal configuration dip switches,
     * this value specifies their positions as a bitmap. Dip switches are
     * generally used to indicate the presence of additional hardware which
     * will be represented by other cookies.
     */
    if (has_dip_switches)
    {
        setvalue_swi();
        cookie_add(COOKIE_SWI, cookie_swi);
    }
#endif

    /* _SND
     * This cookie contains a bitmap of sound features available to the
     * system as follows:
     * 0x01 GI Sound Chip (PSG)
     * 0x02 1 Stereo 8-bit Playback
     * 0x04 DMA Record (w/XBIOS)
     * 0x08 16-bit CODEC
     * 0x10 DSP
     */
    setvalue_snd();
    cookie_add(COOKIE_SND, cookie_snd);

#if CONF_WITH_FRB
    /* _FRB  This cookie is present when alternative RAM is present. It
     * points to a 64k buffer that may be used by DMA device drivers to
     * transfer memory between alternative RAM and ST RAM for DMA operations.
     */
    KDEBUG(("add_cookie_frb()\n"));
    add_cookie_frb();
#endif

    /* _FLK  The presence of this cookie indicates that file and record
     * locking extensions to GEMDOS exist. The value field is a version
     * number currently undefined.
     */

    /* _AKP  This cookie indicates the presence of an Advanced Keyboard
     * Processor. The high word of this cookie is currently reserved.
     * The low word indicates the language currently used by TOS for
     * keyboard interpretation and alerts.
     */
    detect_akp();
    KDEBUG(("cookie_akp = 0x%08lx\n", cookie_akp));
    cookie_add(COOKIE_AKP, cookie_akp);

    /* _IDT  This cookie defines the currently configured date and time
     * format.  Bits #0-7 contain the ASCII code of the date separator.
     * Bits #8-11 contain a value indicating the date display format as
     * follows:
     *   0 MM-DD-YY
     *   1 DD-MM-YY
     *   2 YY-MM-DD
     *   3 YY-DD-MM
     * Bits #12-15 contain a value indicating the time format as follows:
     *   0 12 hour
     *   1 24 hour
     * Note: The value of this cookie does not affect any of the internal
     * time functions. It is intended for informational use by applications
     * and may also used by the desktop for its date & time displays.
     */
    detect_idt();
    KDEBUG(("cookie_idt = 0x%08lx\n", cookie_idt));
    cookie_add(COOKIE_IDT, cookie_idt);

#if CONF_WITH_FDC
    /* Floppy Drive Controller
     * Most significant byte means:
     * 0 - DD (Normal floppy interface)
     * 1 - HD (1.44 MB with 3.5")
     * 2 - ED (2.88 MB with 3.5")
     * the 3 other bytes are the Controller ID:
     * 0 - No information available
     * 'ATC' - Fully compatible interface built in a way that
     * behaves like part of the system.
     */
    setvalue_fdc();
    cookie_add(COOKIE_FDC, cookie_fdc);
#endif

#if DETECT_NATIVE_FEATURES
    if (has_natfeats())
    {
        cookie_add(COOKIE_NATFEAT, (ULONG)&natfeat_cookie);
    }
#endif

#if CONF_WITH_XHDI
    cookie_add(COOKIE_XHDI, (ULONG)xhdi_vec);
#endif

#if CONF_WITH_SCSI_DRIVER
    cookie_add(COOKIE_SCSIDRIV, (ULONG)&scsidriv_root);
#endif

#if !CONF_WITH_MFP
    /* Set the _5MS cookie with the address of the 200 Hz system timer
     * interrupt vector so FreeMiNT can hook it. */
    cookie_add(COOKIE__5MS, (ULONG)&vector_5ms);
#endif
}

static const char * guess_machine_name(void)
{
#if CONF_WITH_ARANYM
    if (is_aranym)
        return aranym_name;
#endif

    switch(cookie_mch) {
    case MCH_ST:
        if (HAS_MEGARTC)
            return "Atari Mega ST";
        else
            return "Atari ST";
    case MCH_STE:
        return "Atari STe";
    case MCH_MSTE:
        return "Atari Mega STe";
    case MCH_TT:
        return "Atari TT";
    case MCH_FALCON:
        return "Atari Falcon";
    default:
        return "unknown";
    }
}

const char * machine_name(void)
{
    MAYBE_UNUSED(guess_machine_name);
#ifdef MACHINE_FIREBEE
    return "FireBee";
#elif defined(MACHINE_AMIGA)
    return amiga_machine_name();
#elif defined(MACHINE_LISA)
    return "Apple Lisa";
#elif defined(MACHINE_M548X)
    return m548x_machine_name();
#elif defined(MACHINE_A2560U) || defined(MACHINE_A2560X)
    struct foenix_system_info_t info;

    a2560_system_info(&info);
 
    return info.model_name;
#else

    return guess_machine_name();
#endif
}
