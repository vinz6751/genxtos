/*
 * emutos.ld - Custom linker script for EmuTOS ROM
 *
 * Copyright (C) 2016-2020 The EmuTOS development team
 *
 * Authors:
 *  VRI   Vincent Rivière
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

/* Note: this linker script is preprocessed,
 * to allow #include, #define, #if, etc. */
#include "include/config.h"

/* Create a plain binary file, without any header */
OUTPUT_FORMAT(binary)

INCLUDE linker/memorymap.ld

SECTIONS
{
    INCLUDE linker/section_first_stram.ld
    INCLUDE linker/section_alt_ram.ld

    /* This section is the internal BIOS stack.
     * It will *not* be cleared on startup or reset.
     */
    .stack :
    {
        _stkbot = .;
        . += 2K;
        _stktop = .;
    } >REGION_READ_WRITE

    /* This section is located as low as possible in ST-RAM,
     * but after eventual BIOS stack.
     * Variables requiring very low addresses, while being accessible
     * from user mode, can be put here.
     */
    .low_stram : SUBALIGN(2)
    {
        __low_stram_start = .;
#if 0
        obj/lowstram.o(.bss COMMON)
        obj/lineavars.o(.bss COMMON)
        __low_stram_end = .;
        ASSERT(ABSOLUTE(_shifty) < 0x8000, "error: bad _shifty address");
        ASSERT((ABSOLUTE(_dskbuf) & 3) == 0, "error: bad _dskbuf alignment");
        ASSERT(ABSOLUTE(__low_stram_end) <= 0x8000, "error: .low_stram section extends beyond 32k");
#endif
    } >stram

    INCLUDE linker/section_text.ld

    /* FIXME: Our DATA segment is currently read-only.
     * It currently lives in the ROM, just after the TEXT segment.
     * This means that initialized global variables can't be modified.
     * Since a read-only DATA segment is useless, we try to keep it empty by
     * making all initialized variables const, so they go to the TEXT segment.
     */
    .data : SUBALIGN(2)
    {
        __data = .;
        *(.data)
        __edata = .;
    } >REGION_READ_ONLY

    INCLUDE linker/section_bss.ld

    /* This section is always the last one stored in ST-RAM.
     * It is usually empty, just used to calculate the last address
     * of ST-RAM statically used by EmuTOS.
     */
    .laststram :
    {
        __end_os_stram = .;
    } >stram

#if CONF_WITH_STATIC_ALT_RAM
    /* Last section in static Alt-RAM */
    .last_altram :
    {
        __static_altram_end = .;
    } >altram
#endif

    /* Discard the following ELF sections.
     * Some of them may be present in ELF libgcc.a.
     */
    /DISCARD/ :
    {
        *(.comment)
        *(.debug*)
    }
}

#ifdef ELF_TOOLCHAIN
/* Our code is compiled with -fleading-underscore, so the references to external
 * libraries will include a leading underscore. However, libgcc was not compiled
 * with that option, so its symbols do not start with a leading underscore. So
 * we need to define leading-underscore aliases to the no-leading-underscore
 * functions in libgcc.
 */
#define DEFINE_SYMBOL_ALIAS(alias, impl) EXTERN(impl) PROVIDE(alias = impl);
#define ELF_LIB_REF(symbol) DEFINE_SYMBOL_ALIAS(_##symbol, symbol)
ELF_LIB_REF(__mulsi3)
ELF_LIB_REF(__divsi3)
ELF_LIB_REF(__modsi3)
ELF_LIB_REF(__udivsi3)
ELF_LIB_REF(__umodsi3)
#endif
