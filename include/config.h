/*
 * config.h - default settings
 *
 * When adding a configuration option to this file, you should ensure
 * (via #ifdef) that it can be overridden by entries in localconf.h.
 * Defines that should *not* be overridden should appear in sysconf.h
 * (or deskconf.h if they apply to EmuDesk).
 *
 * Copyright (C) 2001-2025 The EmuTOS development team
 *
 * Authors:
 *  MAD     Martin Doering
 *  LVL     Laurent Vogel
 *  VRI     Vincent Rivière
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#ifndef _CONFIG_H
#define _CONFIG_H

/*
 * File localconf.h will be included if reported present by the Makefile.
 * Use it to put your local configuration. File localconf.h will not be
 * imported into Git.
 */

#ifdef LOCALCONF
# include "../localconf.h"
#endif

/*
 * Determine if this EmuTOS is built for ROM or RAM
 */
#if defined(TARGET_PRG) || defined(TARGET_FLOPPY) || defined(TARGET_AMIGA_FLOPPY) || defined(TARGET_LISA_FLOPPY)
#  define EMUTOS_LIVES_IN_RAM 1
# else
#  define EMUTOS_LIVES_IN_RAM 0
#endif

/*
 * Determine if we use static Alt-RAM
 */
#ifdef STATIC_ALT_RAM_ADDRESS
# ifdef CONF_WITH_STATIC_ALT_RAM
#  error CONF_WITH_STATIC_ALT_RAM must not be manually set. Define STATIC_ALT_RAM_ADDRESS instead.
# endif
# define CONF_WITH_STATIC_ALT_RAM 1
#endif



/****************************************************
 *  T A R G E T   D E F A U L T S   S E C T I O N   *
 ****************************************************/

/*
 * Defaults for the ARAnyM target
 */
#ifdef MACHINE_ARANYM
# ifndef CONF_WITH_VDI_16BIT
#  define CONF_WITH_VDI_16BIT 1
# endif
# ifndef CONF_WITH_APOLLO_68080
#  define CONF_WITH_APOLLO_68080 0
# endif
# ifndef CONF_WITH_IKBD_CLOCK
#  define CONF_WITH_IKBD_CLOCK 0
# endif
# ifndef CONF_WITH_CARTRIDGE
#  define CONF_WITH_CARTRIDGE 0
# endif
# ifndef CONF_WITH_ACSI
#  define CONF_WITH_ACSI 0
# endif
# ifndef CONF_WITH_SCSI
#  define CONF_WITH_SCSI 0
# endif
# ifndef CONF_WITH_TT_MFP
#  define CONF_WITH_TT_MFP 0
# endif
# ifndef CONF_WITH_STE_SHIFTER
#  define CONF_WITH_STE_SHIFTER 0
# endif
# ifndef CONF_WITH_TT_SHIFTER
#  define CONF_WITH_TT_SHIFTER 0
# endif
# ifndef CONF_WITH_SCC
#  define CONF_WITH_SCC 1
# endif
# ifndef CONF_WITH_MEGARTC
#  define CONF_WITH_MEGARTC 0
# endif
# ifndef CONF_WITH_VME
#  define CONF_WITH_VME 0
# endif
# ifndef CONF_WITH_SFP004
#  define CONF_WITH_SFP004 0
# endif
# ifndef CONF_WITH_68030_PMMU
#  define CONF_WITH_68030_PMMU 0
# endif
# ifndef CONF_WITH_68040_PMMU
#  define CONF_WITH_68040_PMMU 1
# endif
# ifndef CONF_WITH_MONSTER
#  define CONF_WITH_MONSTER 0
# endif
# ifndef CONF_WITH_MAGNUM
#  define CONF_WITH_MAGNUM 0
# endif
# ifndef CONF_WITH_NOVA
#  define CONF_WITH_NOVA 0
# endif
# ifndef CONF_WITH_FORMAT
#  define CONF_WITH_FORMAT 0
# endif
# ifndef CONF_WITH_EASTER_EGG
#  define CONF_WITH_EASTER_EGG 0 /* ARAnyM's YM2149 can't produce sound */
# endif
#endif

/*
 * Defaults for the FireBee target
 */
#ifdef MACHINE_FIREBEE
# ifndef CONF_WITH_ST_MMU
#  define CONF_WITH_ST_MMU 0
# endif
# ifndef CONF_WITH_TT_MMU
#  define CONF_WITH_TT_MMU 0
# endif
# ifndef CONF_WITH_TT_MFP
#  define CONF_WITH_TT_MFP 0
# endif
# ifndef CONF_WITH_STE_SHIFTER
#  define CONF_WITH_STE_SHIFTER 0
# endif
# ifndef CONF_WITH_TT_SHIFTER
#  define CONF_WITH_TT_SHIFTER 0
# endif
# ifndef CONF_WITH_DSP
#  define CONF_WITH_DSP 0
# endif
# ifndef CONF_WITH_SCC
#  define CONF_WITH_SCC 0
# endif
# ifndef CONF_WITH_MEGARTC
#  define CONF_WITH_MEGARTC 0
# endif
# ifndef CONF_WITH_BLITTER
#  define CONF_WITH_BLITTER 0
# endif
# ifndef CONF_WITH_CACHE_CONTROL
#  define CONF_WITH_CACHE_CONTROL 0
# endif
# ifndef CONF_WITH_SFP004
#  define CONF_WITH_SFP004 0
# endif
# ifndef CONF_WITH_68030_PMMU
#  define CONF_WITH_68030_PMMU 0
# endif
# ifndef CONF_WITH_68040_PMMU
#  define CONF_WITH_68040_PMMU 0
# endif
# ifndef CONF_WITH_SDMMC
#  define CONF_WITH_SDMMC 1
# endif
# ifndef CONF_DETECT_FIRST_BOOT_WITHOUT_MEMCONF
#  define CONF_DETECT_FIRST_BOOT_WITHOUT_MEMCONF 1
# endif
# ifndef INITINFO_DURATION
#  define INITINFO_DURATION 8 /* Longer time for LCD monitors startup */
# endif
# ifndef AES_STACK_SIZE
#  define AES_STACK_SIZE 2048   /* in LONGs */
# endif
# ifndef CONF_WITH_ACSI
#  define CONF_WITH_ACSI 0      /* broken in current FireBee hardware */
# endif
# ifndef CONF_WITH_SCSI
#  define CONF_WITH_SCSI 0
# endif
# ifndef CONF_WITH_ICDRTC
#  define CONF_WITH_ICDRTC 0    /* useless on FireBee as it has NVRAM clock */
# endif
# ifndef CONF_WITH_ULTRASATAN_CLOCK
#  define CONF_WITH_ULTRASATAN_CLOCK 0    /* useless on FireBee as it has NVRAM clock */
# endif
# ifndef CONF_WITH_MONSTER
#  define CONF_WITH_MONSTER 0
# endif
# ifndef CONF_WITH_MAGNUM
#  define CONF_WITH_MAGNUM 0
# endif
# ifndef CONF_WITH_NOVA
#  define CONF_WITH_NOVA 0
# endif
# ifndef CONF_WITH_FORMAT
#  define CONF_WITH_FORMAT 0
# endif
#endif

/*
 * Defaults for the 192 target.
 * This target is only useful on ST hardware, and the ROM size is very
 * limited, so strip out all the advanced features.
 */
#ifdef TARGET_192
# ifndef TOS_VERSION
/* Some software checks for TOS version < 1.06 to determine that the ROM
 * is located at 0x00fc0000. So we use 1.04 as it is the most recent TOS
 * version located there. */
#  define TOS_VERSION 0x104
# endif
# ifndef DETECT_NATIVE_FEATURES
#  define DETECT_NATIVE_FEATURES 0
# endif
# ifndef CONF_WITH_ADVANCED_CPU
#  define CONF_WITH_ADVANCED_CPU 0
# endif
# ifndef CONF_WITH_APOLLO_68080
#  define CONF_WITH_APOLLO_68080 0
# endif
# ifndef CONF_WITH_CACHE_CONTROL
#  define CONF_WITH_CACHE_CONTROL 0
# endif
# ifndef CONF_WITH_TT_MMU
#  define CONF_WITH_TT_MMU 0
# endif
# ifndef CONF_WITH_FALCON_MMU
#  define CONF_WITH_FALCON_MMU 0
# endif
# ifndef CONF_WITH_ALT_RAM
#  define CONF_WITH_ALT_RAM 0
# endif
# ifndef CONF_WITH_MONSTER
#  define CONF_WITH_MONSTER 0
# endif
# ifndef CONF_WITH_MAGNUM
#  define CONF_WITH_MAGNUM 0
# endif
# ifndef CONF_WITH_NOVA
#  define CONF_WITH_NOVA 0
# endif
# ifndef CONF_WITH_TTRAM
#  define CONF_WITH_TTRAM 0
# endif
# ifndef CONF_WITH_TT_MFP
#  define CONF_WITH_TT_MFP 0
# endif
# ifndef CONF_WITH_SCC
#  define CONF_WITH_SCC 0
# endif
# ifndef CONF_WITH_SCSI
#  define CONF_WITH_SCSI 0
# endif
# ifndef CONF_WITH_IDE
#  define CONF_WITH_IDE 0
# endif
# ifndef CONF_WITH_SCSI_DRIVER
#  define CONF_WITH_SCSI_DRIVER 0
# endif
# ifndef CONF_WITH_STE_SHIFTER
#  define CONF_WITH_STE_SHIFTER 0
# endif
# ifndef CONF_WITH_TT_SHIFTER
#  define CONF_WITH_TT_SHIFTER 0
# endif
# ifndef CONF_WITH_VIDEL
#  define CONF_WITH_VIDEL 0
# endif
# ifndef CONF_WITH_DMASOUND
#  define CONF_WITH_DMASOUND 0
# endif
# ifndef CONF_WITH_XBIOS_SOUND
#  define CONF_WITH_XBIOS_SOUND 0
# endif
# ifndef CONF_WITH_DSP
#  define CONF_WITH_DSP 0
# endif
# ifndef CONF_WITH_VME
#  define CONF_WITH_VME 0
# endif
# ifndef CONF_WITH_DIP_SWITCHES
#  define CONF_WITH_DIP_SWITCHES 0
# endif
# ifndef CONF_WITH_NVRAM
#  define CONF_WITH_NVRAM 0
# endif
# ifndef CONF_WITH_ICDRTC
#  define CONF_WITH_ICDRTC 0
# endif
# ifndef CONF_WITH_ULTRASATAN_CLOCK
#  define CONF_WITH_ULTRASATAN_CLOCK 0
# endif
# ifndef CONF_WITH_XHDI
#  define CONF_WITH_XHDI 0
# endif
# ifndef CONF_WITH_COLOUR_ICONS
#  define CONF_WITH_COLOUR_ICONS 0
# endif
# ifndef CONF_WITH_GRAF_MOUSE_EXTENSION
#  define CONF_WITH_GRAF_MOUSE_EXTENSION 0
# endif
# ifndef CONF_WITH_WINDOW_COLOURS
#  define CONF_WITH_WINDOW_COLOURS 0
# endif
# ifndef CONF_WITH_3D_OBJECTS
#  define CONF_WITH_3D_OBJECTS 0
# endif
# ifndef CONF_WITH_EXTENDED_OBJECTS
#  define CONF_WITH_EXTENDED_OBJECTS 0
# endif
# ifndef CONF_WITH_MENU_EXTENSION
#  define CONF_WITH_MENU_EXTENSION 0
# endif
# ifndef CONF_WITH_NICELINES
#  define CONF_WITH_NICELINES 0
# endif
# ifndef CONF_WITH_WINDOW_ICONS
#  define CONF_WITH_WINDOW_ICONS 0
# endif
# ifndef CONF_WITH_DESKTOP_SHORTCUTS
#  define CONF_WITH_DESKTOP_SHORTCUTS 0
# endif
# ifndef CONF_WITH_BACKGROUNDS
#  define CONF_WITH_BACKGROUNDS 0
# endif
# ifndef CONF_WITH_SEARCH
#  define CONF_WITH_SEARCH 0
# endif
#ifndef CONF_WITH_SIZE_TO_FIT
# define CONF_WITH_SIZE_TO_FIT 0
#endif
# ifndef CONF_WITH_BOTTOMTOTOP
#  define CONF_WITH_BOTTOMTOTOP 0
# endif
# ifndef CONF_WITH_SELECTALL
#  define CONF_WITH_SELECTALL 0
# endif
# ifndef CONF_WITH_FILEMASK
#  define CONF_WITH_FILEMASK 0
# endif
# ifndef CONF_WITH_ALT_DESKTOP_GRAPHICS
#  define CONF_WITH_ALT_DESKTOP_GRAPHICS 0
# endif
# ifndef CONF_WITH_DESKTOP_CONFIG
#  define CONF_WITH_DESKTOP_CONFIG 0
# endif
# ifndef CONF_WITH_PCGEM
#  define CONF_WITH_PCGEM 0
# endif
# ifndef CONF_WITH_BIOS_EXTENSIONS
#  define CONF_WITH_BIOS_EXTENSIONS 0
# endif
# ifndef CONF_WITH_EXTENDED_MOUSE
#  define CONF_WITH_EXTENDED_MOUSE 0
# endif
# ifndef CONF_WITH_VDI_16BIT
#  define CONF_WITH_VDI_16BIT 0
# endif
# ifndef CONF_WITH_VDI_VERTLINE
#  define CONF_WITH_VDI_VERTLINE 0
# endif
# ifndef CONF_WITH_SHOW_FILE
#  define CONF_WITH_SHOW_FILE 0
# endif
# ifndef CONF_WITH_PRINTER_ICON
#  define CONF_WITH_PRINTER_ICON 0
# endif
# ifndef CONF_WITH_READ_INF
#  define CONF_WITH_READ_INF 0
# endif
# ifndef CONF_WITH_68030_PMMU
#  define CONF_WITH_68030_PMMU 0
# endif
# ifndef CONF_WITH_68040_PMMU
#  define CONF_WITH_68040_PMMU 0
# endif
# ifndef CONF_WITH_FORMAT
#  define CONF_WITH_FORMAT 0
# endif
# ifndef CONF_WITH_SHUTDOWN
#  define CONF_WITH_SHUTDOWN 0
# endif
# ifndef MAX_VERTICES
#  define MAX_VERTICES 512
# endif
# ifndef NUM_VDI_HANDLES
#  define NUM_VDI_HANDLES 64
# endif
#endif

/*
 * Defaults for the 256 target.
 * This target is only useful on (Mega)STe hardware, or for checking
 * language-dependent features under Hatari.  We disable features that
 * are not known (and not likely) to be available for that hardware.
 */
#ifdef TARGET_256
# ifndef CONF_WITH_ARANYM
#  define CONF_WITH_ARANYM 0
# endif
# ifndef CONF_WITH_APOLLO_68080
#  define CONF_WITH_APOLLO_68080 0
# endif
# ifndef CONF_WITH_FALCON_MMU
#  define CONF_WITH_FALCON_MMU 0
# endif
# ifndef CONF_WITH_68030_PMMU
#  define CONF_WITH_68030_PMMU 0
# endif
# ifndef CONF_WITH_NVRAM
#  define CONF_WITH_NVRAM 0
# endif
# ifndef CONF_WITH_SCSI
#  define CONF_WITH_SCSI 0
# endif
# ifndef CONF_WITH_TT_MFP
#  define CONF_WITH_TT_MFP 0
# endif
# ifndef CONF_WITH_TT_MMU
#  define CONF_WITH_TT_MMU 0
# endif
# ifndef CONF_WITH_TT_SHIFTER
#  define CONF_WITH_TT_SHIFTER 0
# endif
# ifndef CONF_WITH_VIDEL
#  define CONF_WITH_VIDEL 0
# endif
# ifndef CONF_WITH_DSP
#  define CONF_WITH_DSP 0
# endif
# ifndef CONF_WITH_ALT_DESKTOP_GRAPHICS
#  define CONF_WITH_ALT_DESKTOP_GRAPHICS 0
# endif
# ifndef CONF_WITH_3D_OBJECTS
#  define CONF_WITH_3D_OBJECTS 0
# endif
# ifndef CONF_WITH_MENU_EXTENSION
#  define CONF_WITH_MENU_EXTENSION 0
# endif
# ifndef CONF_WITH_VDI_16BIT
#  define CONF_WITH_VDI_16BIT 0
# endif
# ifndef MAX_VERTICES
#  define MAX_VERTICES 512
# endif
#endif

/*
 * Defaults for the diagnostic cartridge target (maximum size 128K).
 * When this is selected, the Makefile excludes AES support in order
 * to reduce ROM size.  However this is still insufficient, so we
 * need to exclude some feature(s).  The cartridge will still run on
 * standard Atari systems, with the following restrictions:
 *  . for the TT:
 *      . SCSI is not available, you must use ACSI or add-on IDE
 *  . for the Falcon:
 *      . SCSI is not available, you must use IDE
 *      . DSP is not supported
 *  . for all systems:
 *      . DMA sound is not supported
 *      . alternate/TT RAM is not supported
 *      . the 68040 PMMU is not supported
 *      . the Apollo 68080 is not supported
 *      . support for add-on cards such as MonSTer, Magnum is disabled
 *      . extended mouse functions (extra buttons etc) are not supported
 */
#ifdef TARGET_CART
# ifndef DIAGNOSTIC_CARTRIDGE
#  define DIAGNOSTIC_CARTRIDGE 1
# endif
# ifndef DETECT_NATIVE_FEATURES
#  define DETECT_NATIVE_FEATURES 0
# endif
# ifndef CONF_WITH_APOLLO_68080
#  define CONF_WITH_APOLLO_68080 0
# endif
# ifndef CONF_WITH_CACHE_CONTROL
#  define CONF_WITH_CACHE_CONTROL 0
# endif
# ifndef CONF_WITH_ALT_RAM
#  define CONF_WITH_ALT_RAM 0
# endif
# ifndef CONF_WITH_TTRAM
#  define CONF_WITH_TTRAM 0
# endif
# ifndef CONF_WITH_SCSI
#  define CONF_WITH_SCSI 0
# endif
# ifndef CONF_WITH_SCSI_DRIVER
#  define CONF_WITH_SCSI_DRIVER 0
# endif
# ifndef CONF_WITH_TT_MFP
#  define CONF_WITH_TT_MFP 0
# endif
# ifndef CONF_WITH_TT_SHIFTER
#  define CONF_WITH_TT_SHIFTER 0
# endif
# ifndef CONF_WITH_BIOS_EXTENSIONS
#  define CONF_WITH_BIOS_EXTENSIONS 0
# endif
# ifndef CONF_WITH_EXTENDED_MOUSE
#  define CONF_WITH_EXTENDED_MOUSE 0
# endif
# ifndef CONF_WITH_VDI_16BIT
#  define CONF_WITH_VDI_16BIT 0
# endif
# ifndef CONF_WITH_VDI_TEXT_SPEEDUP
#  define CONF_WITH_VDI_TEXT_SPEEDUP 0
# endif
# ifndef CONF_WITH_VDI_VERTLINE
#  define CONF_WITH_VDI_VERTLINE 0
# endif
# ifndef CONF_WITH_DMASOUND
#  define CONF_WITH_DMASOUND 0
# endif
# ifndef CONF_WITH_XBIOS_SOUND
#  define CONF_WITH_XBIOS_SOUND 0
# endif
# ifndef CONF_WITH_DSP
#  define CONF_WITH_DSP 0
# endif
# ifndef CONF_WITH_XHDI
#  define CONF_WITH_XHDI 0
# endif
# ifndef CONF_WITH_ICDRTC
#  define CONF_WITH_ICDRTC 0
# endif
# ifndef CONF_WITH_ULTRASATAN_CLOCK
#  define CONF_WITH_ULTRASATAN_CLOCK 0
# endif
# ifndef CONF_WITH_68040_PMMU
#  define CONF_WITH_68040_PMMU 0
# endif
# ifndef CONF_WITH_SHUTDOWN
#  define CONF_WITH_SHUTDOWN 0
# endif
# ifndef CONF_WITH_MONSTER
#  define CONF_WITH_MONSTER 0
# endif
# ifndef CONF_WITH_MAGNUM
#  define CONF_WITH_MAGNUM 0
# endif
# ifndef CONF_WITH_NOVA
#  define CONF_WITH_NOVA 0
# endif
# ifndef MAX_VERTICES
#  define MAX_VERTICES 512
# endif
# ifndef NUM_VDI_HANDLES
#  define NUM_VDI_HANDLES 64
# endif
#endif

/*
 * Defaults for the standard floppy target
 */
#ifdef TARGET_FLOPPY
# ifndef MAX_VERTICES
#  define MAX_VERTICES 512
# endif
#endif

/*
 * Defaults for the Amiga ROM target
 */
#ifdef TARGET_AMIGA_ROM
# define MACHINE_AMIGA
#endif

/*
 * Defaults for Amiga floppy targets
 */
#ifdef TARGET_AMIGA_FLOPPY
# define MACHINE_AMIGA
#endif

/*
 * Defaults for the Amiga machine
 */
#ifdef MACHINE_AMIGA
# ifndef CONF_ATARI_HARDWARE
#  define CONF_ATARI_HARDWARE 0
# endif
# ifndef CONF_WITH_IDE
#  define CONF_WITH_IDE 1
# endif
# ifndef CONF_WITH_UAE
#  define CONF_WITH_UAE 1
# endif
#endif

/*
 * Defaults for the Apple Lisa floppy target
 */
#ifdef TARGET_LISA_FLOPPY
# define MACHINE_LISA
#endif

/*
 * Defaults for the Apple Lisa machine
 */
#ifdef MACHINE_LISA
# ifndef CONF_ATARI_HARDWARE
#  define CONF_ATARI_HARDWARE 0
# endif
# ifndef CONF_WITH_ADVANCED_CPU
#  define CONF_WITH_ADVANCED_CPU 0
# endif
# ifndef CONF_WITH_APOLLO_68080
#  define CONF_WITH_APOLLO_68080 0
# endif
# ifndef CONF_WITH_CACHE_CONTROL
#  define CONF_WITH_CACHE_CONTROL 0
# endif
# ifndef CONF_WITH_EJECT
#  define CONF_WITH_EJECT 1
# endif
# ifndef USE_STOP_INSN_TO_FREE_HOST_CPU
   /* This makes LisaEm timings completely inaccurate, so disable it */
#  define USE_STOP_INSN_TO_FREE_HOST_CPU 0
# endif
#endif

/*
 * Defaults for the M548x machine
 */
#ifdef MACHINE_M548X
# ifndef CONF_ATARI_HARDWARE
#  define CONF_ATARI_HARDWARE 0
# endif
# ifndef CONF_STRAM_SIZE
#  define CONF_STRAM_SIZE 14*1024*1024
# endif
# ifndef CONF_TTRAM_SIZE
#  define CONF_TTRAM_SIZE 48UL*1024*1024
# endif
# ifndef CONF_SERIAL_CONSOLE
#  define CONF_SERIAL_CONSOLE 1
# endif
# ifndef CONF_WITH_IDE
#  define CONF_WITH_IDE 1
# endif
# ifndef CONF_WITH_SDMMC
#  define CONF_WITH_SDMMC 1
# endif
# ifndef CONF_WITH_FLEXCAN
#  define CONF_WITH_FLEXCAN 1
# endif
# ifndef CONF_DETECT_FIRST_BOOT_WITHOUT_MEMCONF
#  define CONF_DETECT_FIRST_BOOT_WITHOUT_MEMCONF 1
# endif
# ifndef ALWAYS_SHOW_INITINFO
#  define ALWAYS_SHOW_INITINFO 1
# endif
# ifndef AES_STACK_SIZE
#  define AES_STACK_SIZE 2048   /* in LONGs */
# endif
#endif

/*
 * Defaults for the C256 Foenix GenX machine with 68000 CPU Module
 */
#ifdef MACHINE_C256FOENIXGENX
# ifndef CONF_ATARI_HARDWARE
#  define CONF_ATARI_HARDWARE 0
# endif
# ifndef CONF_WITH_IDE
#  define CONF_WITH_IDE 0 /* Not supported yet */
# endif
# ifndef CONF_DETECT_FIRST_BOOT_WITHOUT_MEMCONF
#  define CONF_DETECT_FIRST_BOOT_WITHOUT_MEMCONF 0
# endif
# ifdef WITH_AES
#  undef WITH_AES
# endif
# define WITH_AES 0 /* Not supported yet */
# ifndef WITH_CLI
#  define WITH_CLI 1 /* Doesn't work yet but we need to startup something... */
# endif
#endif


/*
 * Defaults for the A2560U from Foenix Retro Systems
 */
#ifdef MACHINE_A2560U
# define FOENIX_CHANNEL_A_DEBUG_PRINT 0
# ifndef MACHINE_A2560_DEBUG
#  define MACHINE_A2560_DEBUG 0
# endif
# ifndef CONF_ATARI_HARDWARE
#  define CONF_ATARI_HARDWARE 0
# endif
# ifndef CONF_WITH_IDE
#  define CONF_WITH_IDE 1
# endif
# ifndef CONF_WITH_SDMMC
#  define CONF_WITH_SDMMC 1
# endif
# ifndef CONF_DETECT_FIRST_BOOT_WITHOUT_MEMCONF
#  define CONF_DETECT_FIRST_BOOT_WITHOUT_MEMCONF 0
# endif
# ifndef WITH_AES
# define WITH_AES 0 /* Not supported yet */
# endif
# ifndef WITH_CLI
#  define WITH_CLI 1
# endif
# ifndef CONF_WITH_SN76489
#  define CONF_WITH_SN76489 1
# endif
# ifndef CONF_WITH_WM8776
#  define CONF_WITH_WM8776 1
# endif
# ifndef CONF_WITH_BQ4802LY
#  define CONF_WITH_BQ4802LY 1
# endif
# ifndef CONF_WITH_MPU401
#  define CONF_WITH_MPU401 0
# endif
#if 0
# ifndef CONF_VRAM_ADDRESS
#  define CONF_VRAM_ADDRESS 0x00c00000 /* VRAM is at a special location */
# endif
#endif
# ifndef CONF_WITH_CHUNKY8
#  define CONF_WITH_CHUNKY8 1
# endif
/* At least one of CONF_WITH_A2560_TEXT_MODE and CONF_WITH_A2560U_SHADOW_FRAMEBUFFER must be enabled */
/* Use VICKY's text mode if possible rather than a bitmap screen buffer when using 8 pixel-high font.
 * No graphics possible. */
#define CONF_WITH_A2560_TEXT_MODE 1
/* Shadow framebuffer support (e.g. for rendering 8x16). Safe/recommended to leave enabled. */
#define CONF_WITH_A2560U_SHADOW_FRAMEBUFFER 1
# ifndef CONF_WITH_FORCE_8x8_FONT
#  define CONF_WITH_FORCE_8x8_FONT 1
# endif
# ifndef ALWAYS_SHOW_INITINFO
#  define ALWAYS_SHOW_INITINFO 1 /* So we can get into EmuCON */
# endif
# ifdef USE_STOP_INSN_TO_FREE_HOST_CPU /* To confirm: this causes a crash so we don't want it */
#  undef USE_STOP_INSN_TO_FREE_HOST_CPU
#  define USE_STOP_INSN_TO_FREE_HOST_CPU 0
# endif
#ifdef DEFAULT_BAUDRATE
#  undef DEFAULT_BAUDRATE
#  define DEFAULT_BAUDRATE 12 /*38400*/
#endif
#ifdef MACHINE_A2560_DEBUG
# define CONF_WITH_EXTENDED_MOUSE 0 /* Not supported (yet?) */
# define RS232_DEBUG_PRINT 1
#endif
#endif

/*
 * Defaults for the A2560X from Foenix Retro Systems
 */
#ifdef MACHINE_A2560X
# ifndef MACHINE_A2560_DEBUG
#  define MACHINE_A2560_DEBUG 1
# endif
# ifndef FOENIX_CHANNEL_A_DEBUG_PRINT
#  define FOENIX_CHANNEL_A_DEBUG_PRINT 0
# endif
# ifndef CONF_ATARI_HARDWARE
#  define CONF_ATARI_HARDWARE 0
# endif
# ifndef CONF_WITH_IDE
#  define CONF_WITH_IDE 1
# endif
# ifndef CONF_WITH_SDMMC
#  define CONF_WITH_SDMMC 1
# endif
# ifndef CONF_DETECT_FIRST_BOOT_WITHOUT_MEMCONF
#  define CONF_DETECT_FIRST_BOOT_WITHOUT_MEMCONF 0
# endif
# ifndef WITH_AES
# define WITH_AES 0 /* Not supported yet */
# endif
# ifndef WITH_CLI
#  define WITH_CLI 1
# endif
# ifndef CONF_WITH_SN76489
#  define CONF_WITH_SN76489 1
# endif
# ifndef CONF_WITH_WM8776
#  define CONF_WITH_WM8776 1
# endif
# ifndef CONF_WITH_BQ4802LY
#  define CONF_WITH_BQ4802LY 1
# endif
# ifndef CONF_WITH_MPU401
#  define CONF_WITH_MPU401 1
# endif
#if 0
# ifndef CONF_VRAM_ADDRESS
#  define CONF_VRAM_ADDRESS 0x00c00000 /* VRAM is at a special location */
# endif
#endif
# ifndef CONF_WITH_CHUNKY8
#  define CONF_WITH_CHUNKY8 1
# endif
/* At least one of CONF_WITH_A2560_TEXT_MODE and CONF_WITH_A2560X_SHADOW_FRAMEBUFFER must be enabled */
/* Use VICKY's text mode if possible rather than a bitmap screen buffer when using 8 pixel-high font.
 * No graphics possible. */
#define CONF_WITH_A2560_TEXT_MODE 1
/* Shadow framebuffer support (e.g. for rendering 8x16). Safe/recommended to leave enabled. */
#define CONF_WITH_A2560X_SHADOW_FRAMEBUFFER 0
# ifndef CONF_WITH_FORCE_8x8_FONT
#  define CONF_WITH_FORCE_8x8_FONT 1
# endif
# ifndef ALWAYS_SHOW_INITINFO
#  define ALWAYS_SHOW_INITINFO 1 /* So we can get into EmuCON */
# endif
# ifdef USE_STOP_INSN_TO_FREE_HOST_CPU /* To confirm: this causes a crash so we don't want it */
#  undef USE_STOP_INSN_TO_FREE_HOST_CPU
#  define USE_STOP_INSN_TO_FREE_HOST_CPU 0
# endif
#ifdef DEFAULT_BAUDRATE
#  undef DEFAULT_BAUDRATE
#  define DEFAULT_BAUDRATE 12 /*38400*/
#endif
#ifdef MACHINE_A2560_DEBUG
# define CONF_WITH_EXTENDED_MOUSE 0 /* Not supported (yet?) */
# define RS232_DEBUG_PRINT 0
#endif
#endif


/*
 * By default, EmuTOS is built for Atari ST/TT/Falcon compatible hardware
 */
#ifndef CONF_ATARI_HARDWARE
# define CONF_ATARI_HARDWARE 1
#endif

/*
 * Defaults for non-Atari hardware.
 * Disable all Atari specific hardware.
 */
#if !CONF_ATARI_HARDWARE
# ifndef DETECT_NATIVE_FEATURES
#  define DETECT_NATIVE_FEATURES 0
# endif
# ifndef CONF_WITH_ST_MMU
#  define CONF_WITH_ST_MMU 0
# endif
# ifndef CONF_WITH_TT_MMU
#  define CONF_WITH_TT_MMU 0
# endif
# ifndef CONF_WITH_FALCON_MMU
#  define CONF_WITH_FALCON_MMU 0
# endif
# ifndef CONF_WITH_FRB
#  define CONF_WITH_FRB 0
# endif
# if !(defined(CONF_WITH_TTRAM) || defined(CONF_TTRAM_SIZE))
#  define CONF_WITH_TTRAM 0
# endif
# ifndef CONF_WITH_MFP
#  define CONF_WITH_MFP 0
# endif
# ifndef CONF_WITH_MFP_RS232
#  define CONF_WITH_MFP_RS232 0
# endif
# ifndef CONF_WITH_TT_MFP
#  define CONF_WITH_TT_MFP 0
# endif
# ifndef CONF_WITH_SCC
#  define CONF_WITH_SCC 0
# endif
# ifndef CONF_WITH_YM2149
#  define CONF_WITH_YM2149 0
# endif
# ifndef CONF_WITH_PRINTER_PORT
#  define CONF_WITH_PRINTER_PORT 0
# endif
# ifndef CONF_WITH_MIDI_ACIA
#  define CONF_WITH_MIDI_ACIA 0
# endif
# ifndef CONF_WITH_IKBD_ACIA
#  define CONF_WITH_IKBD_ACIA 0
# endif
# ifndef CONF_WITH_IKBD_CLOCK
#  define CONF_WITH_IKBD_CLOCK 0
# endif
# ifndef CONF_WITH_CARTRIDGE
#  define CONF_WITH_CARTRIDGE 0
# endif
# ifndef CONF_WITH_FDC
#  define CONF_WITH_FDC 0
# endif
# ifndef CONF_WITH_FORMAT
#  define CONF_WITH_FORMAT 0
# endif
# ifndef CONF_WITH_ACSI
#  define CONF_WITH_ACSI 0
# endif
# ifndef CONF_WITH_SCSI
#  define CONF_WITH_SCSI 0
# endif
# ifndef CONF_WITH_IDE
#  define CONF_WITH_IDE 0
# endif
# ifndef CONF_WITH_SCSI_DRIVER
#  define CONF_WITH_SCSI_DRIVER 0
# endif
# ifndef CONF_WITH_ATARI_VIDEO
#  define CONF_WITH_ATARI_VIDEO 0
# endif
# ifndef CONF_WITH_STE_SHIFTER
#  define CONF_WITH_STE_SHIFTER 0
# endif
# ifndef CONF_WITH_TT_SHIFTER
#  define CONF_WITH_TT_SHIFTER 0
# endif
# ifndef CONF_WITH_VIDEL
#  define CONF_WITH_VIDEL 0
# endif
# ifndef CONF_WITH_MEGARTC
#  define CONF_WITH_MEGARTC 0
# endif
# ifndef CONF_WITH_DMASOUND
#  define CONF_WITH_DMASOUND 0
# endif
# ifndef CONF_WITH_XBIOS_SOUND
#  define CONF_WITH_XBIOS_SOUND 0
# endif
# ifndef CONF_WITH_DSP
#  define CONF_WITH_DSP 0
# endif
# ifndef CONF_WITH_VME
#  define CONF_WITH_VME 0
# endif
# ifndef CONF_WITH_DIP_SWITCHES
#  define CONF_WITH_DIP_SWITCHES 0
# endif
# ifndef CONF_WITH_NVRAM
#  define CONF_WITH_NVRAM 0
# endif
# ifndef CONF_WITH_BLITTER
#  define CONF_WITH_BLITTER 0
# endif
# ifndef CONF_WITH_SFP004
#  define CONF_WITH_SFP004 0
# endif
# ifndef CONF_WITH_68030_PMMU
#  define CONF_WITH_68030_PMMU 0
# endif
# ifndef CONF_WITH_68040_PMMU
#  define CONF_WITH_68040_PMMU 0
# endif
# ifndef CONF_WITH_BUS_ERROR
#  define CONF_WITH_BUS_ERROR 0
# endif
# ifndef CONF_WITH_MONSTER
#  define CONF_WITH_MONSTER 0
# endif
# ifndef CONF_WITH_MAGNUM
#  define CONF_WITH_MAGNUM 0
# endif
# ifndef CONF_WITH_NOVA
#  define CONF_WITH_NOVA 0
# endif
# ifndef CONF_WITH_ALT_DESKTOP_GRAPHICS
#  define CONF_WITH_ALT_DESKTOP_GRAPHICS 0 /* Like ST, not Falcon */
# endif
# ifndef CONF_WITH_3D_OBJECTS
#  define CONF_WITH_3D_OBJECTS 0 /* Like ST, not Falcon */
# endif
# ifndef CONF_WITH_SN767489
#  define CONF_WITH_SN767489 0
# endif
# ifndef CONF_WITH_MPU401
#  define CONF_WITH_MPU401 0
# endif
# ifndef CONF_WITH_CHUNKY8
#  define CONF_WITH_CHUNKY8 0
# endif
# ifndef CONF_WITH_FORCE_8x8_FONT
#  define CONF_WITH_FORCE_8x8_FONT 0
# endif
# ifndef CONF_WITH_A2560_TEXT_MODE
#  define CONF_WITH_A2560_TEXT_MODE 0
# endif
# ifndef CONF_WITH_A2560U_SHADOW_FRAMEBUFFER
#  define CONF_WITH_A2560U_SHADOW_FRAMEBUFFER 0
# endif
# ifndef CONF_WITH_VDI_16BIT
#  define CONF_WITH_VDI_16BIT 0 /* Like ST, not Falcon */
# endif
#endif

/*
 * Set DIAGNOSTIC_CARTRIDGE to 1 when building a diagnostic cartridge
 */
#ifndef DIAGNOSTIC_CARTRIDGE
# define DIAGNOSTIC_CARTRIDGE 0
#endif



/*
 * use #ifndef ... #endif for definitions below, to allow them to
 * be overridden by the Makefile or by localconf.h
 */



/********************************************************
 *  H A R D W A R E   S U P P O R T   S E C T I O N     *
 ********************************************************/

/*
 * Set CONF_WITH_ADVANCED_CPU to 1 to enable support for 68010-68060
 */
#ifndef CONF_WITH_ADVANCED_CPU
# ifdef __mcoldfire__
#  define CONF_WITH_ADVANCED_CPU 0
# else
#  define CONF_WITH_ADVANCED_CPU 1
# endif
#endif

/*
 * Set CONF_WITH_APOLLO_68080 to 1 to enable support for Apollo 68080 CPU
 */
#ifndef CONF_WITH_APOLLO_68080
# ifdef __mcoldfire__
#  define CONF_WITH_APOLLO_68080 0
# else
#  define CONF_WITH_APOLLO_68080 1
# endif
#endif

/*
 * Set CONF_WITH_ST_MMU to 1 to enable support for ST MMU
 */
#ifndef CONF_WITH_ST_MMU
# define CONF_WITH_ST_MMU 1
#endif

/*
 * Set CONF_WITH_TT_MMU to 1 to enable support for TT MMU
 */
#ifndef CONF_WITH_TT_MMU
# define CONF_WITH_TT_MMU 1
#endif

/*
 * Set CONF_WITH_FALCON_MMU to 1 to enable support for Falcon MMU
 */
#ifndef CONF_WITH_FALCON_MMU
# define CONF_WITH_FALCON_MMU 1
#endif

/*
 * Define CONF_STRAM_SIZE to the actual size of the ST-RAM, in bytes.
 * If set to 0, the amount of ST-RAM will be autodetected.
 */
#ifndef CONF_STRAM_SIZE
# define CONF_STRAM_SIZE 0
#endif

/*
 * Set CONF_WITH_ALT_RAM to 1 to add support for alternate RAM
 */
#ifndef CONF_WITH_ALT_RAM
# define CONF_WITH_ALT_RAM 1
#endif

/*
 * Set CONF_WITH_STATIC_ALT_RAM to 1 to put EmuTOS static data in Alt-RAM.
 * This can only work if the main Alt-RAM address is known at compile time.
 * The actual Alt-RAM region is described in emutos.ld.
 */
#ifndef CONF_WITH_STATIC_ALT_RAM
# define CONF_WITH_STATIC_ALT_RAM 0
#endif

/*
 * Set CONF_WITH_TTRAM to 1 to enable detection and usage of TT-RAM
 */
#ifndef CONF_WITH_TTRAM
# define CONF_WITH_TTRAM 1
#endif

/*
 * Define CONF_TTRAM_SIZE to the actual size of the TT-RAM, in bytes.
 * If set to 0, the amount of TT-RAM will be autodetected.
 */
#ifndef CONF_TTRAM_SIZE
# define CONF_TTRAM_SIZE 0
#endif

/*
 * Define CONF_WITH_MONSTER to enable detection and usage of ST/STE
 * MonSTer expansion card
 */
#ifndef CONF_WITH_MONSTER
# define CONF_WITH_MONSTER 1
#endif

/*
 * Define CONF_WITH_MAGNUM to enable detection and usage of Magnum
 * RAM expansion card
 */
#ifndef CONF_WITH_MAGNUM
# define CONF_WITH_MAGNUM 1
#endif

/*
 * Set CONF_WITH_MFP to 1 to enable support for the MFP 68901
 */
#ifndef CONF_WITH_MFP
# define CONF_WITH_MFP 1
#endif

/*
 * Set CONF_WITH_MFP_RS232 to 1 to enable MFP RS-232 support
 */
#ifndef CONF_WITH_MFP_RS232
# define CONF_WITH_MFP_RS232 1
#endif

/*
 * Set CONF_WITH_TT_MFP to 1 to enable TT MFP support
 */
#ifndef CONF_WITH_TT_MFP
# define CONF_WITH_TT_MFP 1
#endif

/*
 * Set CONF_WITH_SCC to 1 to enable SCC support
 */
#ifndef CONF_WITH_SCC
# define CONF_WITH_SCC 1
#endif

/*
 * Set CONF_COLDFIRE_TIMER_C to 1 to simulate Timer C using the
 * internal ColdFire timers
 */
#ifndef CONF_COLDFIRE_TIMER_C
# if defined(__mcoldfire__) && !CONF_WITH_MFP
#  define CONF_COLDFIRE_TIMER_C 1
# else
#  define CONF_COLDFIRE_TIMER_C 0
# endif
#endif

/*
 * Set CONF_WITH_COLDFIRE_RS232 to 1 to use the internal ColdFire serial port
 */
#ifndef CONF_WITH_COLDFIRE_RS232
# ifdef __mcoldfire__
#  define CONF_WITH_COLDFIRE_RS232 1
# else
#  define CONF_WITH_COLDFIRE_RS232 0
# endif
#endif

/*
 * Set CONF_WITH_YM2149 to 1 to enable YM2149 soundchip support
 */
#ifndef CONF_WITH_YM2149
# define CONF_WITH_YM2149 1
#endif

/*
 * Set CONF_WITH_PRINTER_PORT to 1 to enable Parallel Printer Port support
 */
#ifndef CONF_WITH_PRINTER_PORT
# define CONF_WITH_PRINTER_PORT 1
#endif

/*
 * Set CONF_WITH_MIDI_ACIA to 1 to enable MIDI ACIA support
 */
#ifndef CONF_WITH_MIDI_ACIA
# define CONF_WITH_MIDI_ACIA 1
#endif

/*
 * Set CONF_WITH_IKBD_ACIA to 1 to enable IKBD ACIA support
 */
#ifndef CONF_WITH_IKBD_ACIA
# define CONF_WITH_IKBD_ACIA 1
#endif

/*
 * Set CONF_WITH_IKBD_CLOCK to 1 to enable IKBD clock support
 */
#ifndef CONF_WITH_IKBD_CLOCK
# define CONF_WITH_IKBD_CLOCK 1
#endif

/*
 * Set CONF_WITH_CARTRIDGE to 1 to enable ROM port cartridge support
 */
#ifndef CONF_WITH_CARTRIDGE
# if DIAGNOSTIC_CARTRIDGE
   /* Diagnostic and Application cartridges have different magic numbers,
    * so a diagnostic cartridge can't also be an application cartridge. */
#  define CONF_WITH_CARTRIDGE 0
# else
#  define CONF_WITH_CARTRIDGE 1
# endif
#endif

/*
 * Set CONF_WITH_FDC to 1 to enable floppy disk controller support
 */
#ifndef CONF_WITH_FDC
# define CONF_WITH_FDC 1
#endif

/*
 * Set this to 1 to activate ACSI support
 */
#ifndef CONF_WITH_ACSI
# define CONF_WITH_ACSI 1
#endif

/*
 * Set CONF_WITH_SCSI to 1 to activate SCSI support
 */
#ifndef CONF_WITH_SCSI
# define CONF_WITH_SCSI 1
#endif

/*
 * Set CONF_WITH_IDE to 1 to activate Falcon IDE support
 */
#ifndef CONF_WITH_IDE
# define CONF_WITH_IDE 1
#endif

/*
 * Set CONF_WITH_SDMMC to 1 to activate SD/MMC bus support
 */
#ifndef CONF_WITH_SDMMC
# define CONF_WITH_SDMMC 0
#endif

/*
 * Set CONF_WITH_VAMPIRE_SPI to 1 to activate SPI on the Vampire,
 * required for SD/MMC support on these boards
 */
#ifndef CONF_WITH_VAMPIRE_SPI
# define CONF_WITH_VAMPIRE_SPI 0
#endif

/*
 * Set CONF_WITH_ATARI_VIDEO to 1 to enable support for ST Shifter and higher
 */
#ifndef CONF_WITH_ATARI_VIDEO
# define CONF_WITH_ATARI_VIDEO 1
#endif

/*
 * Set CONF_WITH_STE_SHIFTER to 1 to enable support for STe Shifter
 */
#ifndef CONF_WITH_STE_SHIFTER
# define CONF_WITH_STE_SHIFTER 1
#endif

/*
 * Set CONF_WITH_TT_SHIFTER to 1 to enable support for TT Shifter
 */
#ifndef CONF_WITH_TT_SHIFTER
# define CONF_WITH_TT_SHIFTER 1
#endif

/*
 * Set CONF_WITH_VIDEL to 1 to enable support for Falcon Videl
 */
#ifndef CONF_WITH_VIDEL
# define CONF_WITH_VIDEL 1
#endif

/*
 * CONF_VRAM_ADDRESS allows the video ram address to be set to a fixed
 * location, outside ST-RAM or Alt-RAM. This allows the use of custom
 * graphic cards.  Set to 0 to allocate the video ram in ST-RAM as usual.
 */
#ifndef CONF_VRAM_ADDRESS
# define CONF_VRAM_ADDRESS 0
#endif

/*
 * Set CONF_WITH_MEGARTC to 1 to enable MegaST real-time clock support
 */
#ifndef CONF_WITH_MEGARTC
# define CONF_WITH_MEGARTC 1
#endif

/*
 * Set CONF_WITH_ICDRTC to 1 to enable support for the real-time clock on
 * the ICD AdSCSI Plus ST board, which is attached via the ACSI interface.
 * You must also enable CONF_WITH_ACSI.
 */
# ifndef CONF_WITH_ICDRTC
#  define CONF_WITH_ICDRTC CONF_WITH_ACSI
# endif

/*
 * Set CONF_WITH_ULTRASATAN_CLOCK to 1 to enable ULTRASATAN clock support
 * Based on CONF_WITH_ACSI value
 */
#ifndef CONF_WITH_ULTRASATAN_CLOCK
# define CONF_WITH_ULTRASATAN_CLOCK CONF_WITH_ACSI
#endif

/*
 * Set CONF_WITH_DMASOUND to 1 to enable support for STe/TT/Falcon DMA sound
 */
#ifndef CONF_WITH_DMASOUND
# define CONF_WITH_DMASOUND 1
#endif

/*
 * Set CONF_WITH_DSP to 1 to enable support for Falcon DSP
 */
#ifndef CONF_WITH_DSP
# define CONF_WITH_DSP 1
#endif

/*
 * Set CONF_WITH_VME to 1 to enable support for the Mega STe VME bus
 */
#ifndef CONF_WITH_VME
# define CONF_WITH_VME 1
#endif

/*
 * Set CONF_WITH_DIP_SWITCHES to 1 to enable support for STe/TT/Falcon
 * DIP switches
 */
#ifndef CONF_WITH_DIP_SWITCHES
# define CONF_WITH_DIP_SWITCHES 1
#endif

/*
 * Set CONF_WITH_NVRAM to 1 to enable NVRAM support
 */
#ifndef CONF_WITH_NVRAM
# define CONF_WITH_NVRAM 1
#endif

/*
 * Set CONF_WITH_BLITTER to 1 to enable blitter support
 */
#ifndef CONF_WITH_BLITTER
# define CONF_WITH_BLITTER 1
#endif

/*
 * Set CONF_WITH_SFP004 to 1 to enable 68881 FPU support for the Mega ST
 */
#ifndef CONF_WITH_SFP004
# define CONF_WITH_SFP004 1
#endif

/*
 * Set CONF_WITH_NOVA to 1 to enable support for Nova graphic card adapter
 */
#ifndef CONF_WITH_NOVA
# define CONF_WITH_NOVA 1
#endif

/* Set CONF_WITH_FLEXCAN to 1 to enable support for the FlexCAN controller.
 * This allows use of an Eiffel keyboard adapter plugged into the CAN port
 * of ColdFire evaluation boards.
 */
#ifndef CONF_WITH_FLEXCAN
# define CONF_WITH_FLEXCAN 0
#endif

/*
 * Set CONF_WITH_RESET to 0 to force the startup code to bypass the
 * "reset" instruction during startup.  By default it is bypassed
 * in EmuTOS RAM, because it causes crashes very early in startup (the
 * "black screen" problem).  It is surmised that the hardware reset may
 * reset the RAM controller allowing/causing RAM contents to change.
 * It is also bypassed in ColdFire because there is no reset instruction.
 */
#ifndef CONF_WITH_RESET
# if EMUTOS_LIVES_IN_RAM || defined(__mcoldfire__)
#  define CONF_WITH_RESET 0
# else
#  define CONF_WITH_RESET 1
# endif
#endif

/*
 * Set CONF_WITH_BAS_MEMORY_MAP to 1 if EmuTOS is intended to run
 * over the BaS (either on the FireBee or on the M548x with BaS_gcc)
 */
#ifndef CONF_WITH_BAS_MEMORY_MAP
# ifdef MACHINE_FIREBEE
#  define CONF_WITH_BAS_MEMORY_MAP 1
# else
#  define CONF_WITH_BAS_MEMORY_MAP 0
# endif
#endif



/****************************************************
 *  S O F T W A R E   S E C T I O N   -   A E S     *
 ****************************************************/

/*
 * AES_STACK_SIZE is the size of the private stack for each AES process,
 * specified in LONGs. It is used for the AES itself, including each
 * call to the VDI, BIOS and GEMDOS. In typical usage, the operation
 * requiring the most stack space is running FreeMiNT with GEM=ROM, and
 * double-clicking xaloader.prg to run XaAES. That calls EmuTOS's
 * appl_init() (to determine if the physical VDI workstation is open),
 * which ends up calling Fsfirst().  In this situation, this is FreeMiNT's
 * Fsfirst() which uses about 1.5kB of stack space.
 *
 * NOTE: an application that calls v_gtext() via a USERDEF (e.g. using
 * CFlib), and links to Gemlib to provide v_gtext(), will need a large AES
 * stack, since Gemlib's v_gtext() implementation puts a 1024-word buffer
 * on the stack. In order to run such programs, we use a large stack.
 * Existing 68K-compatible TOS programs will have worked around this problem,
 * otherwise they would not run on standard Atari TOS. Thus this is
 * principally a problem when recompiling for ColdFire systems, and so
 * we default to a larger value when building for them (see above).
 *
 * A value for AES_STACK_SIZE can be estimated by enabling the define
 * CONF_DEBUG_AES_STACK (see "Debug section" below).
 */
#ifndef AES_STACK_SIZE
# define AES_STACK_SIZE 590     /* standard value for 68K systems, in LONGs */
#endif

/*
 * Set CONF_WITH_3D_OBJECTS to 1 to enable support for 3D objects,
 * as in Atari TOS 4
 */
#ifndef CONF_WITH_3D_OBJECTS
# define CONF_WITH_3D_OBJECTS 1
#endif

/*
 * Set CONF_WITH_COLOUR_ICONS to 1 to enable support for colour icons,
 * as in Atari TOS 4
 */
#ifndef CONF_WITH_COLOUR_ICONS
# define CONF_WITH_COLOUR_ICONS 1
#endif

/*
 * Set CONF_WITH_EXTENDED_OBJECTS to 1 to include AES support for a
 * number of MagiC-style object type extensions
 */
#ifndef CONF_WITH_EXTENDED_OBJECTS
# define CONF_WITH_EXTENDED_OBJECTS 1
#endif

/*
 * Set CONF_WITH_GRAF_MOUSE_EXTENSION to 1 to include AES support for
 * graf_mouse() modes M_SAVE, M_RESTORE, M_PREVIOUS.
 */
#ifndef CONF_WITH_GRAF_MOUSE_EXTENSION
# define CONF_WITH_GRAF_MOUSE_EXTENSION 1
#endif

/*
 * Set CONF_WITH_LOADABLE_CURSORS to 1 to allow mouse cursors to
 * be loaded from the file specified by CURSOR_RSC_NAME
 */
#ifndef CONF_WITH_LOADABLE_CURSORS
# define CONF_WITH_LOADABLE_CURSORS 1
#endif
#if CONF_WITH_LOADABLE_CURSORS
# define CURSOR_RSC_NAME "A:\\EMUCURS.RSC"  /* path to user cursor file */
#endif

/*
 * Set CONF_WITH_MENU_EXTENSION to 1 to include AES support for
 * menu_popup(), menu_attach(), menu_istart(), menu_settings().
 *
 * See the source code comments for the limitations of the current
 * implementation.
 */
#ifndef CONF_WITH_MENU_EXTENSION
# define CONF_WITH_MENU_EXTENSION 1
#endif

/*
 * Set CONF_WITH_NICELINES to use a drawn line instead of dashes for
 * separators in menus
 */
#ifndef CONF_WITH_NICELINES
# define CONF_WITH_NICELINES 1
#endif

/*
 * Set CONF_WITH_PCGEM to 1 to support various PC-GEM-compatible AES functions
 */
#ifndef CONF_WITH_PCGEM
# define CONF_WITH_PCGEM 1
#endif

/*
 * Set CONF_WITH_WINDOW_COLOURS to 1 to include AES support for managing
 * window element colours.  Management is via modes WF_COLOR/WF_DCOLOR
 * in wind_get()/wind_set().
 */
#ifndef CONF_WITH_WINDOW_COLOURS
# define CONF_WITH_WINDOW_COLOURS 1
#endif

/*
 * Define the AES version here. This must be done at the end of the
 * "Software Section - AES", since the value depends on features that
 * are set within that section. Valid values include:
 *      0x0120      AES 1.20, used by TOS v1.02
 *      0x0140      AES 1.40, used by TOS v1.04 & v1.62
 *      0x0320      AES 3.20, used by TOS v2.06 & v3.06
 *      0x0330      AES 3.30, used by TOS v4.00
 *      0x0331      AES 3.31, used by TOS v4.01
 *      0x0340      AES 3.40, used by TOS v4.02 & v4.04
 * Do not change this arbitrarily, as each value implies the presence or
 * absence of certain AES functions ...
 */
#ifndef AES_VERSION
# if CONF_WITH_3D_OBJECTS && CONF_WITH_MENU_EXTENSION && CONF_WITH_WINDOW_COLOURS && CONF_WITH_GRAF_MOUSE_EXTENSION
#  define AES_VERSION 0x0340
# elif CONF_WITH_MENU_EXTENSION && CONF_WITH_WINDOW_COLOURS && CONF_WITH_GRAF_MOUSE_EXTENSION
#  define AES_VERSION 0x0330
# elif CONF_WITH_GRAF_MOUSE_EXTENSION
#  define AES_VERSION 0x0320
# else
#  define AES_VERSION 0x0140
# endif
#endif



/****************************************************
 *  S O F T W A R E   S E C T I O N   -   B I O S   *
 ****************************************************/

/*
 * Define the TOS version here. Valid values are 0x104 and 0x206 for example.
 * This is just a version number, EmuTOS functionality is not affected.
 */
#ifndef TOS_VERSION
/* By default, we pretend to be TOS 2.06, as it is available as an update for
 * both ST and STe. On the other hand, TOS 3.x is only for TT and TOS 4.x
 * only for Falcon. */
# define TOS_VERSION 0x206
#endif

/*
 * By default, the EmuTOS welcome screen (initinfo) is only shown on cold
 * boot.  If you set ALWAYS_SHOW_INITINFO to 1, the welcome screen will
 * always be displayed, on both cold boot and warm boot (reset).
 */
#ifndef ALWAYS_SHOW_INITINFO
# define ALWAYS_SHOW_INITINFO 0
#endif

/*
 * Set FULL_INITINFO to 0 to display the EmuTOS version as a single line
 * of text instead of the full welcome screen.
 * This is only useful when there are severe ROM size restrictions.
 */
#ifndef FULL_INITINFO
# define FULL_INITINFO 1
#endif

/*
 * By default, the EmuTOS welcome screen (initinfo) is displayed for 3
 * seconds. On emulators, this is enough to read the text, and optionally
 * to press Shift to keep the screen displayed. But on real hardware, it
 * can take several seconds for the monitor to recover from stand-by mode,
 * so the welcome screen may never be seen. In such cases, it is wise to
 * increase the welcome screen duration.
 * You can use the INITINFO_DURATION define to specify the welcome screen
 * duration, in seconds. If it is set to 0, the welcome screen will never
 * be displayed.
 */
#ifndef INITINFO_DURATION
# define INITINFO_DURATION 3
#endif

/*
 * Set CONF_DETECT_FIRST_BOOT_WITHOUT_MEMCONF to 1 to detect a first boot
 * even when memconf is not called. This is useful when the RAM is
 * already initialized before EmuTOS boots. This is always the case on
 * the FireBee where the actual RAM is setup by the BaS or other pre-OS.
 * Also, this is always the case when EmuTOS itself lives in RAM.
 */
#ifndef CONF_DETECT_FIRST_BOOT_WITHOUT_MEMCONF
# if EMUTOS_LIVES_IN_RAM
#  define CONF_DETECT_FIRST_BOOT_WITHOUT_MEMCONF 1
# else
#  define CONF_DETECT_FIRST_BOOT_WITHOUT_MEMCONF 0
# endif
#endif

/*
 * Set CONF_SERIAL_CONSOLE to 1 in order to:
 * - send console output to the serial port, in addition to the screen
 * - use exclusively the serial port input for console input.
 */
#ifndef CONF_SERIAL_CONSOLE
# define CONF_SERIAL_CONSOLE 0
#endif

/*
 * Set CONF_SERIAL_CONSOLE_ANSI to 1 if the terminal connected to the
 * serial port uses ANSI escape sequences. Set it to 0 if it is an Atari
 * VT52 terminal.
 */
#ifndef CONF_SERIAL_CONSOLE_ANSI
# if CONF_SERIAL_CONSOLE
#  define CONF_SERIAL_CONSOLE_ANSI 1
# else
#  define CONF_SERIAL_CONSOLE_ANSI 0
# endif
#endif

/*
 * Set CONF_SERIAL_CONSOLE_POLLING_MODE to 1 if ikbdiorec is not filled
 * on serial interrupt when CONF_SERIAL_CONSOLE is enabled. This is handy
 * in early stages when porting EmuTOS to new hardware, as this works even if
 * bconstat1()/bconin1() are implemented by polling. Interrupts are generally
 * more complicated to set up.
 * Pros: Polling mode is good enough for EmuTOS itself.
 * Cons: FreeMiNT's advanced keyboard processor doesn't support polling mode.
 */
#ifndef CONF_SERIAL_CONSOLE_POLLING_MODE
# define CONF_SERIAL_CONSOLE_POLLING_MODE 0
#endif

/*
 * Set CONF_SERIAL_IKBD to 1 to allow IKBD keyboard/mouse/joysticks to be
 * plugged on the serial port
 */
#ifndef CONF_SERIAL_IKBD
# define CONF_SERIAL_IKBD 0
#endif

/*
 * Set CONF_WITH_68030_PMMU to install a PMMU tree on a 68030 CPU.
 * This provides improved performance by allowing the data cache to
 * be enabled.
 */
#ifndef CONF_WITH_68030_PMMU
# define CONF_WITH_68030_PMMU 1
#endif

/*
 * Set CONF_WITH_68040_PMMU to install a PMMU tree when running on a
 * 68040 CPU.  The main purpose of this is to allow FreeMiNT to be
 * run under aranym-mmu without using set_mmu.prg.
 */
#ifndef CONF_WITH_68040_PMMU
# define CONF_WITH_68040_PMMU 0
#endif

/*
 * Set CONF_WITH_BIOS_EXTENSIONS to 1 to support various BIOS extension
 * functions
 */
#ifndef CONF_WITH_BIOS_EXTENSIONS
# define CONF_WITH_BIOS_EXTENSIONS 1
#endif

/*
 * Set CONF_WITH_EXTENDED_MOUSE to 1 to enable extended mouse support.
 * This includes new Eiffel scancodes for mouse buttons 3, 4, 5, and
 * the wheel.
 */
#ifndef CONF_WITH_EXTENDED_MOUSE
# define CONF_WITH_EXTENDED_MOUSE 1
#endif

/*
 * Set CONF_WITH_FRB to 1 to automatically enable the _FRB cookie when required
 */
#ifndef CONF_WITH_FRB
# define CONF_WITH_FRB CONF_WITH_ALT_RAM
#endif

/*
 * set CONF_WITH_MEMORY_TEST to 1 to do a memory test during a cold boot
 */
#ifndef CONF_WITH_MEMORY_TEST
# define CONF_WITH_MEMORY_TEST 0
#endif

/*
 * Set CONF_WITH_XBIOS_SOUND to 1 to enable support for the XBIOS sound
 * extension.  This extension provides (some of) the Falcon XBIOS sound
 * functions when running on STe- or TT-compatible hardware.  You must
 * also enable CONF_WITH_DMASOUND.
 */
#ifndef CONF_WITH_XBIOS_SOUND
# define CONF_WITH_XBIOS_SOUND 1
#endif

/*
 * Set the default baud rate for serial ports.
 */
#ifndef DEFAULT_BAUDRATE
# define DEFAULT_BAUDRATE B9600
#endif

/*
 * Retry count for the internal_inquire() used to detect the presence of
 * a physical hard disk drive
 *
 * Setting this to be non-zero is usually not necessary, and will increase
 * boot time by approximately (0.1 * HD_DETECT_RETRIES * n) seconds, where
 * n is the total number of devices that are not present.  For example, if
 * you set it to 1 on an ST with one device on the ACSI bus, the boot time
 * will increase by (0.1*1*7) = 0.7 seconds.
 *
 * However, a non-zero retry count may help in some cases of misbehaving
 * hardware.
 */
#ifndef HD_DETECT_RETRIES
# define HD_DETECT_RETRIES 0
#endif

/*
 * Set CONF_WITH_1FAT_SUPPORT to 1 to enable support for filesystems with
 * only one file allocation table (FAT) instead of the usual two FATs.
 *
 * This is disabled by default because all versions of Atari TOS assume
 * two FATs. There are erroneously mastered disks that claim to have a
 * single FAT, but in reality have two. For compatibility with Atari TOS
 * EmuTOS has to assume two FATs by default.
 *
 */
#ifndef CONF_WITH_1FAT_SUPPORT
# define CONF_WITH_1FAT_SUPPORT 0
#endif



/********************************************************
 *  S O F T W A R E   S E C T I O N   -   G E M D O S   *
 ********************************************************/

/*
 * Define the GEMDOS version here: this number is returned by the GEMDOS
 * Sversion() function call. The value contains the minor version number
 * in the high-order byte, and the major version number in the low-order
 * byte.  Valid values include:
 *      0x1300      used by TOS v1.0 & v1.02
 *      0x1500      used by TOS v1.04 & v1.06
 *      0x1700      used by TOS v1.62
 *      0x1900      used by TOS v2.01, v2.05, v3.01, v3.05
 *      0x2000      used by TOS v2.06 & v3.06
 *      0x3000      used by TOS v4.0x
 * This does not have a well-defined purpose, although it could be checked
 * by programs to determine presence or absence of certain GEMDOS functions.
 */
#ifndef GEMDOS_VERSION
# define GEMDOS_VERSION 0x2000
#endif

/*
 * CONF_LOGSEC_SIZE defines the maximum size of logical sectors that
 * GEMDOS can handle, specified in bytes; however, if a larger value
 * is found on a mounted drive during startup, that value will be used
 * instead.
 *
 * The value *must* be a power of two between 512 and 16384 inclusive.
 *
 * Note that this is only significant if you use Atari-style FAT
 * filesystems; DOS-style FAT filesystems always have a logical sector
 * size of 512 bytes.
 */
#ifndef CONF_LOGSEC_SIZE
# define CONF_LOGSEC_SIZE 512
#endif



/****************************************************
 *  S O F T W A R E   S E C T I O N   -   V D I     *
 ****************************************************/

/*
 * Set CONF_WITH_GDOS to 1 to generate code within vst_load_fonts() and
 * vst_unload_fonts() that will support GDOS (GDOS intercepts these calls
 * but calls these stubs in the interception routines).
 */
#ifndef CONF_WITH_GDOS
# define CONF_WITH_GDOS 1
#endif

/*
 * Set CONF_WITH_VDI_16BIT to 1 to include VDI support for the Falcon's
 * 16-bit graphics modes.
 */
#ifndef CONF_WITH_VDI_16BIT
# define CONF_WITH_VDI_16BIT 1
#endif

/*
 * Set CONF_WITH_VDI_TEXT_SPEEDUP to 1 to improve some VDI text output
 * performance
 */
#ifndef CONF_WITH_VDI_TEXT_SPEEDUP
# define CONF_WITH_VDI_TEXT_SPEEDUP 1
#endif

/*
 * Set CONF_WITH_VDI_VERTLINE to 1 to improve VDI vertical line drawing
 * performance
 */
#ifndef CONF_WITH_VDI_VERTLINE
# define CONF_WITH_VDI_VERTLINE 1
#endif

/*
 * The VDI functions v_fillarea(), v_pline(), v_pmarker() can handle
 * up to MAX_VERTICES coordinates (MAX_VERTICES/2 points).
 * TOS2 allows 512 vertices, TOS3/TOS4 allow 1024.
 */
#ifndef MAX_VERTICES
# define MAX_VERTICES 1024
#endif

/*
 * VDI configuration
 */
#ifndef NUM_VDI_HANDLES
# define NUM_VDI_HANDLES 128    /* maximum number of open workstations */
#endif



/************************************************************************
 *  S O F T W A R E   S E C T I O N   -   3 R D   P A R T Y   A P I     *
 ************************************************************************/

/*
 * set CONF_WITH_SCSI_DRIVER to 1 to activate support for the SCSI driver
 * API, which allows user programs to issue SCSI-style commands directly
 * to devices.  see the documentation by Steffen Engel for more details.
 */
#ifndef CONF_WITH_SCSI_DRIVER
# define CONF_WITH_SCSI_DRIVER 1
#endif

/*
 * Set CONF_WITH_XHDI to 1 to enable XHDI support (i.e. the XHDI cookie etc.)
 */
#ifndef CONF_WITH_XHDI
# define CONF_WITH_XHDI 1
#endif



/************************************************************
 *  S O F T W A R E   S E C T I O N   -   G E N E R A L     *
 ************************************************************/

/*
 * With this switch you can control if some functions should be used as
 * static-inlines. This is generally a good idea if your compiler supports
 * this (the current GCC does). It will shrink the size of the ROM since
 * only very small functions will be used as static inlines, and it will
 * also make the code faster!
 */
#ifndef USE_STATIC_INLINES
# define USE_STATIC_INLINES 1
#endif



/************************************
 *  D E S K T O P   S E C T I O N   *
 ************************************/

/*
 * in this section, defines are in strict alphabetic sequence for convenience
 */

/*
 * Set CONF_PREFER_STRAM_DISK_BUFFERS to 1 if disk buffers are more efficient
 * when located in ST-RAM (applies to EmuDesk floppy/ACSI DMA transfers)
 */
#ifndef CONF_PREFER_STRAM_DISK_BUFFERS
# if CONF_ATARI_HARDWARE
#  define CONF_PREFER_STRAM_DISK_BUFFERS 1
# else
#  define CONF_PREFER_STRAM_DISK_BUFFERS 0
# endif
#endif

/*
 * Set CONF_WITH_ALT_DESKTOP_GRAPHICS to 1 to replace Atari-style desktop
 * graphic elements with alternate versions:
 *  . left-align dialog titles and draw a line under them
 */
#ifndef CONF_WITH_ALT_DESKTOP_GRAPHICS
# define CONF_WITH_ALT_DESKTOP_GRAPHICS 1
#endif

/*
 * Set CONF_WITH_BACKGROUNDS to 1 to allow the background pattern/colour
 * of the desktop & windows to be configured
 */
#ifndef CONF_WITH_BACKGROUNDS
# define CONF_WITH_BACKGROUNDS 1
#endif

/*
 * Set CONF_WITH_BOTTOMTOTOP to 1 to include 'Cycle windows' in the
 * desktop menu (this is the same as 'Bottom to top' in the Atari desktop)
 */
#ifndef CONF_WITH_BOTTOMTOTOP
# define CONF_WITH_BOTTOMTOTOP 1
#endif

/*
 * Set CONF_WITH_CACHE_CONTROL to 1 to include 'Cache' in the desktop menu
 */
#ifndef CONF_WITH_CACHE_CONTROL
# ifdef __mcoldfire__
#  define CONF_WITH_CACHE_CONTROL 0
# else
#  define CONF_WITH_CACHE_CONTROL 1
# endif
#endif

/*
 * Set CONF_WITH_DESKTOP_CONFIG to 1 to enable the 'Desktop configuration'
 * dialog
 */
#ifndef CONF_WITH_DESKTOP_CONFIG
# define CONF_WITH_DESKTOP_CONFIG 1
#endif

/*
 * Set CONF_WITH_DESKTOP_SHORTCUTS to 1 to allow files & folders to be
 * installed on the desktop as shortcuts
 */
#ifndef CONF_WITH_DESKTOP_SHORTCUTS
# define CONF_WITH_DESKTOP_SHORTCUTS 1
#endif

/*
 * Set CONF_WITH_EASTER_EGG to 1 to include the EmuDesk Easter Egg
 * (this plays a small tune and therefore requires YM2149 support)
 */
#ifndef CONF_WITH_EASTER_EGG
# define CONF_WITH_EASTER_EGG CONF_WITH_YM2149
#endif

/*
 * Set CONF_WITH_FILEMASK to 1 to allow the filemask used for desktop
 * windows to be configured
 */
#ifndef CONF_WITH_FILEMASK
# define CONF_WITH_FILEMASK 1
#endif

/*
 * Set CONF_WITH_FORMAT to 1 to support formatting floppy diskettes in EmuDesk
 */
#ifndef CONF_WITH_FORMAT
# define CONF_WITH_FORMAT 1
#endif

/*
 * Set CONF_WITH_PRINTER_ICON to 1 to support a printer icon in EmuDesk
 */
#ifndef CONF_WITH_PRINTER_ICON
# define CONF_WITH_PRINTER_ICON 1
#endif

/*
 * Set CONF_WITH_READ_INF to 1 to include 'Read .INF file' in the desktop menu
 */
#ifndef CONF_WITH_READ_INF
# define CONF_WITH_READ_INF 1
#endif

/*
 * Set CONF_WITH_SEARCH to 1 to include 'Search' in the desktop menu
 */
#ifndef CONF_WITH_SEARCH
# define CONF_WITH_SEARCH 1
#endif

/*
 * Set CONF_WITH_SELECTALL to 1 to include 'Select all items' in the
 * desktop menu
 */
#ifndef CONF_WITH_SELECTALL
# define CONF_WITH_SELECTALL 1
#endif

/*
 * Set CONF_WITH_SHOW_FILE to 1 to support showing a file in EmuDesk
 * by double-clicking on it
 */
#ifndef CONF_WITH_SHOW_FILE
# define CONF_WITH_SHOW_FILE 1
#endif

/*
 * Set CONF_WITH_SIZE_TO_FIT to 1 to include 'Size to fit' in the
 * desktop menu
 */
#ifndef CONF_WITH_SIZE_TO_FIT
# define CONF_WITH_SIZE_TO_FIT 1
#endif

/*
 * Set CONF_WITH_VIEWER_SUPPORT to 1 to include the ability to define
 * a default viewer for files (invoked if no other application has a
 * matching document type)
 */
#ifndef CONF_WITH_VIEWER_SUPPORT
# define CONF_WITH_VIEWER_SUPPORT 1
#endif

/*
 * Set CONF_WITH_WINDOW_ICONS to 1 to include the icons for files & folders
 */
#ifndef CONF_WITH_WINDOW_ICONS
# define CONF_WITH_WINDOW_ICONS 1
#endif



/****************************************
 *  E M U L A T O R   S E C T I O N     *
 ****************************************/

/*
 * Set CONF_WITH_UAE to 1 to enable support for the advanced features of
 * the UAE emulator on the Amiga target
 */
#ifndef CONF_WITH_UAE
# define CONF_WITH_UAE 0
#endif

/*
 * Set this to 1 if your emulator is capable of emulating properly the
 * STOP opcode (used to reduce host CPU burden during loops).  Set to
 * zero for all emulators which do not properly support the STOP opcode.
 */
#ifndef USE_STOP_INSN_TO_FREE_HOST_CPU
# define USE_STOP_INSN_TO_FREE_HOST_CPU 1
#endif

/*
 * Set STONX_NATIVE_PRINT to 1 if your emulator provides a STonX-like
 * native_print() function, i.e. if the code:
 *   .dc.w 0xa0ff
 *   .dc.l 0
 * executes native function void print_native(char *string);
 */
#ifndef STONX_NATIVE_PRINT
#define STONX_NATIVE_PRINT 0
#endif

/*
 * Set DETECT_NATIVE_FEATURES to 1 to detect and (if detected) use native
 * features provided by the standard "native features" interface
 */
#ifndef DETECT_NATIVE_FEATURES
# ifdef __mcoldfire__
#  define DETECT_NATIVE_FEATURES 0 /* Conflict with ColdFire instructions. */
# else
#  define DETECT_NATIVE_FEATURES 1
# endif
#endif

/*
 * Set CONF_WITH_ARANYM to 1 to enable ARAnyM-specific features and workarounds
 */
#ifndef CONF_WITH_ARANYM
# if DETECT_NATIVE_FEATURES
#  define CONF_WITH_ARANYM 1
# else
#  define CONF_WITH_ARANYM 0
# endif
#endif



/********************************
 *  D E B U G   S E C T I O N   *
 ********************************/

/*
 * Set CONF_DEBUG_AES_STACK to 1 to trace the internal AES stack usage,
 * and estimate the stack requirements
 */
#ifndef CONF_DEBUG_AES_STACK
# define CONF_DEBUG_AES_STACK 0
#endif

/*
 * Set CONF_DEBUG_DESK_STACK to 1 to monitor the desktop stack usage
 */
#ifndef CONF_DEBUG_DESK_STACK
# define CONF_DEBUG_DESK_STACK 0
#endif
#if CONF_DEBUG_DESK_STACK
# define STACK_MARKER 0xdeadbeef
#endif

/*
 * Set CONSOLE_DEBUG_PRINT to 1 to redirect debug prints to the BIOS console
 */
#ifndef CONSOLE_DEBUG_PRINT
# define CONSOLE_DEBUG_PRINT 0
#endif

/*
 * Set RS232_DEBUG_PRINT to 1 to redirect debug prints to MFP RS232 out.
 * This is useful for an emulator without any native debug print capabilities,
 * or for real hardware.
 */
#ifndef RS232_DEBUG_PRINT
# if CONF_SERIAL_CONSOLE && !CONF_WITH_COLDFIRE_RS232
#  define RS232_DEBUG_PRINT 1
# else
#  define RS232_DEBUG_PRINT 0
# endif
#endif

/*
 * Set SCC_DEBUG_PRINT to 1 to redirect debug prints to SCC portB RS232
 * out.  This is primarily for use with real Falcon hardware, which does
 * not use the MFP USART.
 */
#ifndef SCC_DEBUG_PRINT
# define SCC_DEBUG_PRINT 0
#endif

/*
 * Set COLDFIRE_DEBUG_PRINT to 1 to redirect debug prints to the ColdFire serial port
 */
#ifndef COLDFIRE_DEBUG_PRINT
# if CONF_SERIAL_CONSOLE && CONF_WITH_COLDFIRE_RS232
#  define COLDFIRE_DEBUG_PRINT 1
# else
#  define COLDFIRE_DEBUG_PRINT 0
# endif
#endif

/*
 * Set MIDI_DEBUG_PRINT to 1 to redirect debug prints to MIDI out.  This
 * is useful for an emulator without any native debug print capabilities,
 * or for real hardware. This overrides previous debug print settings.
 */
#ifndef MIDI_DEBUG_PRINT
# define MIDI_DEBUG_PRINT 0
#endif

/*
 * Set CARTRIDGE_DEBUG_PRINT to 1 to redirect debug prints to the
 * cartridge port. A character 'c' is encoded into address lines
 * A8-A1 by performing a read access to the upper half of the cartridge
 * address space 0xFB0xxx, which is decoded to the /ROM3 signal.
 * This has the advantage of being always available since it does not
 * require initialization of a peripheral.
 * Output can be decoded by connecting a logic analyzer to A8-A1,
 * triggering on /ROM3, or by using a special firmware for the
 * SidecarTridge Multi-device hardware:
 * https://github.com/czietz/atari-debug-cart
 */
#ifndef CARTRIDGE_DEBUG_PRINT
# define CARTRIDGE_DEBUG_PRINT 0
#endif


/* Determine if kprintf() is available */
#if CONF_WITH_UAE || DETECT_NATIVE_FEATURES || STONX_NATIVE_PRINT || CONSOLE_DEBUG_PRINT || RS232_DEBUG_PRINT || SCC_DEBUG_PRINT || COLDFIRE_DEBUG_PRINT || MIDI_DEBUG_PRINT || CARTRIDGE_DEBUG_PRINT || FOENIX_CHANNEL_A_DEBUG_PRINT
#  define HAS_KPRINTF 1
# else
#  define HAS_KPRINTF 0
#endif

/*
 * Set CONF_WITH_SHUTDOWN to 1 to enable the shutdown() function.
 * It tries to power off the machine, if possible.
 */
#ifndef CONF_WITH_SHUTDOWN
# if DETECT_NATIVE_FEATURES || defined(MACHINE_FIREBEE) || defined(MACHINE_AMIGA) || defined(MACHINE_LISA)
#  define CONF_WITH_SHUTDOWN 1
# else
#  define CONF_WITH_SHUTDOWN 0
# endif
#endif

/*
 * Set CONF_WITH_EJECT to 1 to enable automatic floppy eject.
 * This isn't available on Atari hardware.
 */
#ifndef CONF_WITH_EJECT
# define CONF_WITH_EJECT 0
#endif

/*
 * Set CONF_WITH_BUS_ERROR if the hardware triggers a Bus Error exception
 * when trying to read/write data at an invalid address.
 * This is true on Atari machines.
 */
#ifndef CONF_WITH_BUS_ERROR
# define CONF_WITH_BUS_ERROR 1
#endif

/*
 * Set CONF_WITH_SN76489 if the machine has a SN76489 programmable sound generator.
 * If this is defined, the driver requires SN76489_PORT to be defined as the address
 * of the chip.
 */
#ifndef CONF_WITH_SN76489
# define CONF_WITH_SN76489 0
#endif

/*
 * Set CONF_WITH_WM8776 if the machine has a WM8776 audio codec.
 * If this is defined, the driver requires WM8776_PORT to be defined as the address
 * of the chip.
 */
#ifndef CONF_WITH_WM8776
# define CONF_WITH_WM8776 0
#endif

/*
 * Set CONF_WITH_BQ4802LY if the machine has a bq4802LY real time clock.
 * If this is defined, the driver requires BQ4802LY_PORT to be defined as the address
 * of the chip.
 */
#ifndef CONF_WITH_BQ4802LY
# define CONF_WITH_BQ4802LY 0
#endif

/*
 * Set CONF_WITH_CHUNKY8 if the machine uses a chunky frame buffer with 8 bytes
 * per pixel.
 */
#ifndef CONF_WITH_CHUNKY8
# define CONF_WITH_CHUNKY8 0
#endif

/*
 * Set CONF_WITH_FORCE_8x8_FONT to force the use of the 8x8 font.
 */
#ifndef CONF_WITH_FORCE_8x8_FONT
# define CONF_WITH_FORCE_8x8_FONT 0
#endif

/*
 * Set CONF_WITH_A2560_TEXT_MODE to support the 8x8 text mode supported by VICKY.
 */
#ifndef CONF_WITH_A2560_TEXT_MODE
# define CONF_WITH_A2560_TEXT_MODE 0
#endif

/*
 * Set CONF_WITH_A2560U_SHADOW_FRAMEBUFFER to add support for a shadow framebuffer that
 * is in RAM but copied to VRAM during VBL.
 */
#ifndef CONF_WITH_A2560U_SHADOW_FRAMEBUFFER
# define CONF_WITH_A2560U_SHADOW_FRAMEBUFFER 0
#endif

/*
 * Use the second screen of the Foenix for debug output
 */
# ifndef FOENIX_CHANNEL_A_DEBUG_PRINT
#  define FOENIX_CHANNEL_A_DEBUG_PRINT 0
# endif

/*
 * Whether the system has a MPU-401 MIDI controller (applies to the Foenix)
 */
# ifndef CONF_WITH_MPU401
#  define CONF_WITH_MPU401 0
# endif

/*
 * Useful macros for both assembler and C
 */

#if EMUTOS_LIVES_IN_RAM
/* Offset of a ramtos TEXT symbol defined in obj/ramtos.h */
#define OFFSETOF(x) (x - ADR_TEXT)
#endif



/************************************************
 *  S A N I T Y   C H E C K   S E C T I O N     *
 ************************************************/

/*
 * Sanity checks for features - general
 */

#if EMUTOS_LIVES_IN_RAM
# if DIAGNOSTIC_CARTRIDGE
#  error DIAGNOSTIC_CARTRIDGE is incompatible with EMUTOS_LIVES_IN_RAM.
# endif
#endif

#if !DETECT_NATIVE_FEATURES
# if CONF_WITH_ARANYM
#  error CONF_WITH_ARANYM requires DETECT_NATIVE_FEATURES.
# endif
#endif

#if !CONF_WITH_ADVANCED_CPU
# if CONF_WITH_68030_PMMU
#  error CONF_WITH_68030_PMMU requires CONF_WITH_ADVANCED_CPU.
# endif
# if CONF_WITH_APOLLO_68080
#  error CONF_WITH_APOLLO_68080 requires CONF_WITH_ADVANCED_CPU.
# endif
# if CONF_WITH_CACHE_CONTROL
#  error CONF_WITH_CACHE_CONTROL requires CONF_WITH_ADVANCED_CPU.
# endif
#endif

#if !CONF_WITH_YM2149
# if CONF_WITH_FDC
#  error CONF_WITH_FDC requires CONF_WITH_YM2149.
# endif
#endif

#if !CONF_WITH_ALT_RAM
# if CONF_WITH_STATIC_ALT_RAM
#  error CONF_WITH_STATIC_ALT_RAM requires CONF_WITH_ALT_RAM.
# endif
# if CONF_WITH_TTRAM
#  error CONF_WITH_TTRAM requires CONF_WITH_ALT_RAM.
# endif
#endif

#ifndef STATIC_ALT_RAM_ADDRESS
# if CONF_WITH_STATIC_ALT_RAM
#  error CONF_WITH_STATIC_ALT_RAM requires STATIC_ALT_RAM_ADDRESS.
# endif
# ifdef STATIC_ALT_RAM_SIZE
#  error STATIC_ALT_RAM_SIZE requires STATIC_ALT_RAM_ADDRESS.
# endif
#endif

#if !CONF_WITH_TTRAM
# if CONF_TTRAM_SIZE != 0
#  error CONF_TTRAM_SIZE != 0 requires CONF_WITH_TTRAM.
# endif
#endif

#if !CONF_WITH_MFP
# if CONF_WITH_MFP_RS232
#  error CONF_WITH_MFP_RS232 requires CONF_WITH_MFP.
# endif
# if CONF_WITH_PRINTER_PORT
#  error CONF_WITH_PRINTER_PORT requires CONF_WITH_MFP.
# endif
# if CONF_WITH_FDC
#  error CONF_WITH_FDC requires CONF_WITH_MFP.
# endif
# if CONF_WITH_IKBD_ACIA
#  error CONF_WITH_IKBD_ACIA requires CONF_WITH_MFP.
# endif
# if CONF_WITH_MIDI_ACIA
#  error CONF_WITH_MIDI_ACIA requires CONF_WITH_MFP.
# endif
# if CONF_WITH_SCSI
#  error CONF_WITH_SCSI requires CONF_WITH_MFP.
# endif
#endif

#if !CONF_WITH_ATARI_VIDEO
# if CONF_WITH_STE_SHIFTER
#  error CONF_WITH_STE_SHIFTER requires CONF_WITH_ATARI_VIDEO.
# endif
# if CONF_WITH_TT_SHIFTER
#  error CONF_WITH_TT_SHIFTER requires CONF_WITH_ATARI_VIDEO.
# endif
# if CONF_WITH_VIDEL
#  error CONF_WITH_VIDEL requires CONF_WITH_ATARI_VIDEO.
# endif
#endif

#if !CONF_SERIAL_CONSOLE
# if CONF_SERIAL_CONSOLE_ANSI
#  error CONF_SERIAL_CONSOLE_ANSI requires CONF_SERIAL_CONSOLE.
# endif
#endif

#if !CONF_SERIAL_CONSOLE
# if CONF_SERIAL_CONSOLE_POLLING_MODE
#  error CONF_SERIAL_CONSOLE_POLLING_MODE requires CONF_SERIAL_CONSOLE.
# endif
#endif

#if !CONF_WITH_ACSI
# if CONF_WITH_ICDRTC
#  error CONF_WITH_ICDRTC requires CONF_WITH_ACSI.
# endif
# if CONF_WITH_ULTRASATAN_CLOCK
#  error CONF_WITH_ULTRASATAN_CLOCK requires CONF_WITH_ACSI.
# endif
#endif

#if !CONF_WITH_DMASOUND
# if CONF_WITH_XBIOS_SOUND
#  error CONF_WITH_XBIOS_SOUND requires CONF_WITH_DMASOUND.
# endif
#endif

#if !CONF_WITH_FDC
# if CONF_WITH_FORMAT
#  error CONF_WITH_FORMAT requires CONF_WITH_FDC.
# endif
#endif

#if !CONF_WITH_EXTENDED_OBJECTS
# if CONF_WITH_ALT_DESKTOP_GRAPHICS
#  error CONF_WITH_ALT_DESKTOP_GRAPHICS requires CONF_WITH_EXTENDED_OBJECTS.
# endif
#endif

#if !CONF_WITH_VIDEL
# if CONF_WITH_VDI_16BIT
#  error CONF_WITH_VDI_16BIT requires CONF_WITH_VIDEL
# endif
#endif

/*
 * Sanity checks for debugging options
 */

#if !CONF_WITH_SCC
# if SCC_DEBUG_PRINT
#  error SCC_DEBUG_PRINT requires CONF_WITH_SCC.
# endif
#endif

#if !CONF_WITH_COLDFIRE_RS232
# if COLDFIRE_DEBUG_PRINT
#  error COLDFIRE_DEBUG_PRINT requires CONF_WITH_COLDFIRE_RS232.
# endif
#endif

#if (CONSOLE_DEBUG_PRINT + RS232_DEBUG_PRINT + SCC_DEBUG_PRINT + COLDFIRE_DEBUG_PRINT + MIDI_DEBUG_PRINT + CARTRIDGE_DEBUG_PRINT + FOENIX_CHANNEL_A_DEBUG_PRINT) > 1
# error Only one of CONSOLE_DEBUG_PRINT, RS232_DEBUG_PRINT, SCC_DEBUG_PRINT, COLDFIRE_DEBUG_PRINT, MIDI_DEBUG_PRINT or FOENIX_CHANNEL_A_DEBUG_PRINT must be set to 1.
#endif


/*
 * Sanity checks for features on specific target machines
 */

#if !defined(MACHINE_FIREBEE) && !defined(MACHINE_M548X)
# if CONF_WITH_BAS_MEMORY_MAP
#  error CONF_WITH_BAS_MEMORY_MAP requires MACHINE_FIREBEE or MACHINE_M548X.
# endif
# if CONF_WITH_FLEXCAN
#  error CONF_WITH_FLEXCAN requires MACHINE_FIREBEE or MACHINE_M548X.
# endif
#endif

#ifndef MACHINE_AMIGA
# if CONF_WITH_UAE
#  error CONF_WITH_UAE requires MACHINE_AMIGA.
# endif
#endif

#ifndef MACHINE_ARANYM
# if CONF_WITH_68040_PMMU
#  error CONF_WITH_68040_PMMU requires MACHINE_ARANYM.
# endif
#endif

#endif /* _CONFIG_H */
