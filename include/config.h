/*
 * config.h - default settings
 *
 * Copyright (c) 2001 by Authors:
 *
 *  MAD     Martin Doering
 *  LVL     Laurent Vogel
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#ifndef _CONFIG_H
#define _CONFIG_H

/*
 * File localconf.h will be included if reported present by the Makefile.
 * Use it to put your local configuration. File localconf.h will not be 
 * imported into cvs.
 */

#ifdef LOCALCONF
#include "../localconf.h"
#endif

/*
 * use #ifndef ... #endif for definitions below, to allow them to
 * be overriden by the Makefile or by localconf.h
 */

/* set this to 1 if your emulator provides an STonX-like 
 * native_print() function, i.e. if the code:
 *   dc.w 0xa0ff
 *   dc.l 0
 * executes native function void print_native(char *string);
 */
#ifndef STONX_NATIVE_PRINT
#define STONX_NATIVE_PRINT 0
#endif

/* set this to 1 if you run on ARAnyM >= 0.0.11
 * i.e. if the code:
 *   dc.w 0x7135
 * executes native function void print_native(char *string);
 */
#ifndef ARANYM_NATIVE_PRINT
#define ARANYM_NATIVE_PRINT 0
#endif

/* set this to 1 to try autodetect whether STonX or ARAnyM 
 * native print is available (experimental).
 */
#ifndef DETECT_NATIVE_PRINT
#define DETECT_NATIVE_PRINT 0
#endif

/* set this to 1 if your emulator is capable of emulating properly the 
 * STOP opcode (used to spare host CPU burden during loops). This is
 * currently set to zero as STOP does not work well on all emulators.
 */
#ifndef USE_STOP_INSN_TO_FREE_HOST_CPU
#define USE_STOP_INSN_TO_FREE_HOST_CPU 0
#endif

/* Set this to 1 if you want  Timer-D to be initialized.
 * We don't need Timer-D yet, but some software might depend on an
 * initialized Timer-D. On the other side, a running timer D is yet
 * another task for an emulator, e.g. Hatari slows down with it :-(
 */
#ifndef INIT_TIMER_D
#define INIT_TIMER_D 1
#endif

/* Set this to 1 if you want the EmuTOS ROM to contain only one set
 * of keyboards and fonts. The keyboard and fonts will still be those
 * specified as usual (make COUNTRY=xx). 
 * This is used as an attempt to lower the total size of the ROM
 * for some emulators.
 */

#ifndef CONF_UNIQUE_COUNTRY
#define CONF_UNIQUE_COUNTRY 0
#endif

/* Set this to 1 if you do not want any Native Language Support (NLS)
 * included in EmuTOS. The only language will be default English.
 * This is used as an attempt to lower the total size of the ROM 
 * for some emulators.
 */

#ifndef CONF_NO_NLS
#define CONF_NO_NLS 0
#endif

/* Set this to 1 to activate experimental ACSI support 
 */

#ifndef CONF_WITH_ACSI
#define CONF_WITH_ACSI 1
#endif


/* The keyboard and language are now set using
 *   make LOCALE="xx" 
 * where xx is a lowercase two-letter country code as
 * found in the table in bios/country.c
 */

#endif /* _CONFIG_H */
