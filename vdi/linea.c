/*
 * linea.c - line A graphics initialization
 *
 * Copyright (C) 2001-2022 by Authors:
 *
 * Authors:
 *  MAD  Martin Doering
 *  VB   Vincent Barrilliot
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

 /* #define ENABLE_KDEBUG */

#include "emutos.h"
#include "aesdefs.h"
#include "intmath.h"
#include "linea.h"
#include "lineavars.h"
#include "../bios/screen.h" /* Fixme */
#include "tosvars.h"
#include "a2560u_bios.h"

/*
 * Precomputed value of log2(8/v_planes), used to derive v_planes_shift.
 * Only the indexes 1, 2 and 4 are meaningful (see linea_set_screen_shift()).
 */
static const UBYTE shift_offset[5] = {0, 3, 2, 0, 1};

/*
 * Current shift value, used to speed up calculations; changed when v_planes
 * changes.  To get the address of a pixel x in a scan line in a bit-plane
 * resolution, use the formula: (x&0xfff0)>>v_planes_shift
 */
UBYTE v_planes_shift;

/* Use to clip mouse coordinates */
UWORD screen_max_x;
UWORD screen_max_y;

/* Code to call when the Line A is notified of resolution change. */
void (*linea_on_resolution_changed)(void);


/*
 * linea_init - init linea variables
 */
void linea_init(void)
{
    screen_get_current_mode_info(&v_planes, &V_REZ_HZ, &V_REZ_VT);
    linea_resolution_changed();
    
    KDEBUG(("linea_init(): %dx%d %d-plane (v_lin_wr=%d)\n",
        V_REZ_HZ, V_REZ_VT, v_planes, v_lin_wr));
}


/*
 * set_screen_shift() - sets v_planes_shift from the current value of v_planes
 *
 * . v_planes==8 (used by both Falcon & TT) has a shift value of 0
 *
 * . v_planes==16 indicates Falcon 16-bit mode which does not use bit planes,
 *   so we also set v_planes_shift to 0 (it should not be accessed)
 */
void linea_set_screen_shift(void)
{
    v_planes_shift = (v_planes > 4) ? 0 : shift_offset[v_planes];
}


void linea_resolution_changed(void)
{
    KDEBUG(("linea_resolution_changed v_bas_ad:%p\n", v_bas_ad));

#if CONF_WITH_CHUNKY8
    BYTES_LIN = v_lin_wr = V_REZ_HZ; /* 1 byte per pixel makes it easy */
# if (defined(MACHINE_A2560U) || defined (MACHINE_A2560X) || defined (MACHINE_A2560M)) && CONF_WITH_A2560U_SHADOW_FRAMEBUFFER
    a2560_bios_sfb_setup(v_bas_ad, v_cel_ht);
# endif
#else
    BYTES_LIN = v_lin_wr = V_REZ_HZ / 8 * v_planes;
#endif
    
    linea_max_x = V_REZ_HZ - 1;
    linea_max_y = V_REZ_VT - 1;

    /* precalculate shift value to optimize pixel address calculations */
    linea_set_screen_shift();

    if (linea_on_resolution_changed)
        linea_on_resolution_changed();

    mouse_display_driver.resolution_changed();
}


/*
 * validate colour index
 *
 * checks the supplied colour index and, if valid, returns it;
 * otherwise returns 1 (which by default maps to black)
 */
WORD linea_validate_color_index(WORD colnum)
{
    if ((colnum < 0) || (colnum >= numcolors))
        return 1;

    return colnum;
}

/*
 * get_start_addr - return memory address for column x, row y
 *
 * NOTE: the input x value may be negative (for example, this happens
 * when handling a slanting wideline starting at pixel 0 of a row).  This
 * value must be right-shifted to obtain an offset in bytes.
 * According to the C standard, the result of right-shifting a negative
 * value is implementation-defined.  GCC has the correct behaviour from
 * our point of view: high-order bits are 1-filled, so the number remains
 * negative.
 */
UWORD * get_start_addr(const WORD x, const WORD y)
{
    UBYTE * addr;

    /* init address counter */
    addr = v_bas_ad;                    /* start of screen */

#if CONF_WITH_CHUNKY8
    addr += x;
#else
    WORD x2 = x & 0xfff0;               /* ensure that value to be shifted remains signed! */
    addr += x2 >> v_planes_shift;       /* add x coordinate part of addr */
#endif

    addr += muls(y, v_lin_wr);          /* add y coordinate part of addr */

    return (UWORD*)addr;
}