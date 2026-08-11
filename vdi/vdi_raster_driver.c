/*
 * vdi_raster_driver.c - selection of the VDI screen raster driver
 *
 * See vdi_raster_driver.h for what the driver is responsible for.
 *
 * Copyright (C) 2025 The EmuTOS development team
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

/* #define ENABLE_KDEBUG */

#include "emutos.h"
#include "lineavars.h"
#include "vdi_defs.h"
#include "vdi_raster_driver.h"

/*
 * Deliberately left uninitialised so that it costs .bss rather than .data:
 * vdi_raster_select() is called from linea_resolution_changed(), which the
 * BIOS runs when it sets up the screen, so this is always valid by the time
 * anything can draw.
 */
const VDI_RASTER_DRIVER *vdi_raster;

/*
 * vdi_raster_select - choose the driver matching the current framebuffer
 *
 * Called from linea_resolution_changed(), i.e. at screen init and on every
 * video mode change, once v_planes describes the new mode.
 *
 * The format is not derivable from v_planes alone: 8 bitplanes and chunky
 * 8bpp both report 8 planes.  So the chunky test is compile-time gated,
 * exactly as get_start_addr() does it, and only the drivers a given
 * target can actually use are linked into the ROM.
 */
void vdi_raster_select(void)
{
#if CONF_WITH_CHUNKY8
    /*
     * these machines use a chunky 8bpp framebuffer, except for the packed
     * 1bpp bring-up mode on VICKY B, which is a single bitplane
     */
    if (v_planes == 8)
        vdi_raster = &raster_driver_chunky8;
    else
#endif
#if CONF_WITH_VDI_16BIT
    if (TRUECOLOR_MODE)
        vdi_raster = &raster_driver_atari_truecolor;
    else
#endif
        vdi_raster = &raster_driver_atari_bitplanes;

    KDEBUG(("VDI raster driver: %s (%d planes)\n", vdi_raster->name, v_planes));
}
