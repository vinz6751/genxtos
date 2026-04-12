/* Screen driver for the Foenix Retro Labs A2560's VICKY 2
 * Author: Vincent Barrilliot
 * Public domain
 */

#include "emutos.h"

#if defined(MACHINE_A2560U) || defined(MACHINE_A2560K) || defined(MACHINE_A2560M) || defined(MACHINE_A2560X) || defined(MACHINE_GENX)

#include "biosext.h"
#include "screen.h"
#include "tosvars.h" // v_bas_ad
#include "vectors.h" // int_vbl
#include "../foenix/interrupts.h"
#include "../foenix/regutils.h"
#include "../foenix/shadow_fb.h"
#include "../foenix/vicky2.h"


static void screen_vicky2_screen_init(void)
{
    KDEBUG(("screen_vicky2_screen_init\n"));

    vicky2_init();

    /* Setup VICKY interrupts handler (VBL, HBL etc.) */
    vblsem = 0;

#if CONF_WITH_A2560_SHADOW_FRAMEBUFFER
    a2560_sfb_init();
#endif

    // TODO on the A2560U, are we using channel A since there is no channel B?
    a2560_irq_set_handler(INT_SOF_B, int_vbl);
    KDEBUG(("screen_vicky2_screen_init exiting\n"));
}


static uint32_t screen_vicky2_calc_vram_size(void)
{
    FOENIX_VIDEO_MODE mode;
    vicky2_read_video_mode(vicky, &mode);
    KDEBUG(("a2560_bios_calc_vram_size returns mode:%d, size=%ld\n", mode.id, mode.w * mode. h + EXTRA_VRAM_SIZE));
    return (uint32_t)mode.w * (uint32_t)mode. h + EXTRA_VRAM_SIZE;
}

static void screen_vicky2_get_current_mode_info(uint16_t *planes, uint16_t *hz_rez, uint16_t *vt_rez)
{
    FOENIX_VIDEO_MODE mode;

    vicky2_read_video_mode(vicky, &mode);
    *planes = mode.bpp; /* We have 8bits per pixel */
    *hz_rez = mode.w;
    *vt_rez = mode.h;
    KDEBUG(("screen_vicky2_get_current_mode_info setting hz_rez:%d vt_rez:%d from mode %d\n", *hz_rez, *vt_rez, mode.id));
}


static uint8_t *screen_vicky2_physbase(void)
{
    KDEBUG(("screen_vicky2_physbase: %p\n", vicky2_get_bitmap_address(vicky, 0) + VRAM_Bank0));
    return vicky2_get_bitmap_address(vicky, 0) + VRAM_Bank0;
}


static int16_t screen_vicky2_get_monitor_type(void)
{
    KDEBUG(("screen_vicky2_get_monitor_type\n"));
    return MON_VGA; /* VGA. 5 (DVI) would be more correct but is only known for CT60/Radeon so probably not known about by a lot of software */
}


static int16_t screen_vicky2_check_moderez(int16_t moderez)
{
    return 0;
}

static void screen_vicky2_initialise_palette_registers(WORD rez, WORD mode)
{

}

static int16_t screen_vicky2_screen_can_change_resolution(void)
{
    return FALSE; /* For now... */
}

static WORD screen_vicky2_screen_get_number_of_colors_nuances(void)
{
    /* VICKY colors are 24 bits (if not 32?)  but this function only returns 16 bits and
     * the VDI, EmuDesk etc. don't support more than 256 colors.
     * So we return 0, as TOS 4.04 returns for the VIDEL's true color (32768 colors) mode */
    return 0;
}

static void screen_vicky2_setrez(int16_t rez, int16_t mode)
{
    // TODO map known ST/Falcon (rez,mode) to Foenix
    if (rez != FALCON_REZ)
        return;

    KDEBUG(("screen_vicky2_setrez(%d, %d)\n", rez, mode));
    vicky2_set_video_mode(vicky, mode);
}

static void screen_vicky2_set_palette(const UWORD *new_palette)
{
    WORD i;
    R32(VICKY_B_BG_COLOR) = convert_atari2vicky_color(new_palette[0]);
    i = 16-1-1;
    do {
        vicky2_set_lut_color(vicky, 0, i, convert_atari2vicky_color(new_palette[i]));
    } while (--i >= 0); // Hope for dbra
}

static WORD screen_vicky2_setscreen(UBYTE *logical, const UBYTE *physical, WORD rez, WORD videlmode)
{
    WORD oldmode = 0;

    if ((LONG)logical > 0) {
        v_bas_ad = logical;
        KDEBUG(("v_bas_ad = %p\n", v_bas_ad));
    }
    if ((LONG)physical > 0) {
        screen_setphys(physical);
    }

    /* forbid res changes if Line A variables were 'hacked' or 'rez' is -1 */
    if (rez_was_hacked || (rez == -1)) {
        return 0;
    }

    /* return error for requests for invalid resolutions */
    if ((rez < MIN_REZ) || (rez > MAX_REZ)) {
        KDEBUG(("invalid rez = %d\n", rez));
        return -1;
    }

    vsync();
    screen_vicky2_setrez(rez, videlmode);

    screen_init_services_from_mode_info();

    return oldmode;
}

static void screen_vicky2_setphys(const UBYTE *addr)
{
}

/* This is the whole point of this file */
const SCREEN_DRIVER a2560_screen_driver_vicky2 = {
    screen_vicky2_screen_init,
    screen_vicky2_calc_vram_size,
    screen_vicky2_check_moderez,
    screen_vicky2_initialise_palette_registers,
    screen_vicky2_screen_can_change_resolution,
    screen_vicky2_get_current_mode_info,
    screen_vicky2_setphys,
    screen_vicky2_get_monitor_type,
    screen_vicky2_screen_get_number_of_colors_nuances,
    get_std_pixel_size,
    screen_vicky2_physbase,
    screen_vicky2_setscreen,
    screen_vicky2_set_palette
};

#endif
