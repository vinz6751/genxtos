
/* #define ENABLE_KDEBUG */

#include "emutos.h"

#if defined(MACHINE_A2560M)

#include "biosdefs.h" // MON_MONO
#include "screen.h"
#include "../foenix/regutils.h"
#include "../foenix/vicky2.h"


#define MODEREZ 4 /*1024x768 1bpp*/

static void init(void)
{
    KDEBUG(("screen_vicky3:init\n"));

    // Désactive VICKY2 (valeur qui marche : 0x81)
    R32(VICKY2) = 0x80;

    // Désactive l'écran
    KDEBUG(("VICKY3_CTRL:%p\n",(void*)VICKY3_CTRL));
    R32(VICKY3_CTRL) = 0;
}

static ULONG calc_vram_size(void)
{
    KDEBUG(("screen_vicky3:calc_vram_size\n"));
    return 1024L/8*768;
}

static WORD check_moderez(WORD moderez)
{
    KDEBUG(("screen_vicky3:check_moderez\n"));
    return MODEREZ;
}

static void initialise_palette_registers(WORD rez, WORD mode)
{
    int i;
    KDEBUG(("screen_vicky3:initialise_palette_registers\n"));

    if (MODEREZ == 4 || MODEREZ == 0) {
        // Monochrome modes
        // Couleurs des pixels en mode 1 bit par pixel
        R32(VICKY3+0x8) = 0xffffffL;
        return;
    }

     // Crée une palette avec des dégradés de couleur
    for (i=0; i<256; i++) {
        if (i<64) // Shades of green
            R32(VICKY3+0x2000+i*4) = (i*4L << 16);
        else if (i < 128) // Shades of red
            R32(VICKY3+0x2000+i*4) = (i*4L << 8);
        else if (i < 192) // Shades of blue
            R32(VICKY3+0x2000+i*4) = (i*4L);
        else { // Shades of grey
            LONG v = (i*4) & 255; R32(VICKY3+0x2000+i*4) = (v << 16 | v << 8 | v);
        }
    }
}

static WORD can_change_resolution(void)
{
    KDEBUG(("screen_vicky3:can_change_resolution\n"));
    return FALSE;
}

static void get_current_mode_info(UWORD *planes, UWORD *width, UWORD *height)
{
    KDEBUG(("screen_vicky3:get_current_mode_info\n"));
    *planes = 1;
    *width = 1024;
    *height = 768;
}

static void setphys(const UBYTE *addr)
{
    KDEBUG(("screen_vicky3:setphys(%p)\n",addr));

    // Disable the screen
    R32(VICKY3) = 0;

    R32(VICKY3+0x4) = ((uint32_t)addr)/sizeof(uint32_t); // This is expressed in longs

    // Enable the screen with the video mode
    R32(0xFC000000) = 1 + (MODEREZ << 1);
}

static WORD get_monitor_type(void)
{
    KDEBUG(("screen_vicky3:get_monitor_type\n"));
    return MON_MONO; /* TODO: Would VGA be more accurate (be it for the 60Hzfrequency rather than 72Hz) */
}

static WORD get_number_of_colors_nuances(void)
{
    KDEBUG(("screen_vicky3:get_number_of_colors_nuances\n"));
    return 2;
}

static UBYTE *vicky3_physbase(void)
{
    KDEBUG(("screen_vicky3:vicky3_physbase\n"));
    return (UBYTE *)(R32(VICKY3+0x4)*4);
}

static WORD vicky3_setscreen(UBYTE *logical, const UBYTE *physical, WORD rez, WORD videlmode)
{
    KDEBUG(("screen_vicky3:vicky3_setscreen\n"));
    return MODEREZ;
}

static void set_palette(const UWORD *new_palette)
{
    KDEBUG(("screen_vicky3:set_palette\n"));
    // TODO
}

const SCREEN_DRIVER a2560_screen_driver_vicky3 = {
    init,
    calc_vram_size,
    check_moderez,
    initialise_palette_registers,
    can_change_resolution,
    get_current_mode_info,
    setphys,
    get_monitor_type,
    get_number_of_colors_nuances,
    get_std_pixel_size,
    vicky3_physbase,
    vicky3_setscreen,
    set_palette,
};

#endif
