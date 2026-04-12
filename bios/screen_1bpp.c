
#define ENABLE_KDEBUG

#include "emutos.h"

#if defined(MACHINE_A2560X)

#include "biosdefs.h" // MON_MONO
#include "screen.h"
#include "vectors.h" // int_vbl
#include "../foenix/cpu.h"
#include "../foenix/interrupts.h"
#include "../foenix/regutils.h"
#include "../foenix/vicky2.h"


#define MODEREZ 4 /*1024x768 1bpp*/

/* IRQ handlers */
#if defined(MACHINE_A2560K) || defined(MACHINE_A2560X) || defined(MACHINE_GENX)
void a2560_irq_vicky_a(void); /* VICKY autovector A interrupt handler */
void a2560_irq_vicky_b(void); /* VICKY autovector B interrupt handler */
#elif defined(MACHINE_A2560U) || defined(MACHINE_A2560M)
void a2560_irq_vicky(void); /* Only channel, or channel B on the A2560K/X/GenX */
#else
 #error "Define Foenix machine"
#endif

static void init(void)
{
    KDEBUG(("screen_1bpp:init\n"));
    
#if defined(MACHINE_A2560K) || defined(MACHINE_A2560X) || defined(MACHINE_GENX)
    a2560_irq_set_handler(INT_SOF_B, int_vbl);

    cpu_set_vector(INT_VICKYII_A, (uint32_t)a2560_irq_vicky_a);
    cpu_set_vector(INT_VICKYII_B, (uint32_t)a2560_irq_vicky_b);
#else
    cpu_set_vector(INT_VICKYII, (uint32_t)a2560_irq_vicky);
#endif    
/*
    // Désactive VICKY2 (valeur qui marche : 0x81)
    R32(VICKY2) = 0x80;

    // Désactive l'écran
    KDEBUG(("VICKY3_CTRL:%p\n",(void*)VICKY3_CTRL));
    R32(VICKY3_CTRL) = 0;
    */
}

static ULONG calc_vram_size(void)
{
    KDEBUG(("screen_1bpp:calc_vram_size returning %ld\n",1280L/8*960));
    return 1280L/8*960;
}

static WORD check_moderez(WORD moderez)
{
    KDEBUG(("screen_1bpp:check_moderez\n"));
    return MODEREZ;
}

static void initialise_palette_registers(WORD rez, WORD mode)
{
    int i;
    KDEBUG(("screen_1bpp:initialise_palette_registers\n"));
#if 0
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
#endif
}

static WORD can_change_resolution(void)
{
    KDEBUG(("screen_1bpp:can_change_resolution\n"));
    return FALSE;
}

static void get_current_mode_info(UWORD *planes, UWORD *width, UWORD *height)
{
    KDEBUG(("screen_1bpp:get_current_mode_info\n"));
    *planes = 1;
    *width = 1280;
    *height = 960;
}

static UBYTE *fb;

static void setphys(const UBYTE *addr)
{
    KDEBUG(("screen_1bpp:setphys(%p)\n",addr));
    fb = addr;
#if 0
    // Disable the screen
    R32(VICKY3) = 0;

    R32(VICKY3+0x4) = ((uint32_t)addr)/sizeof(uint32_t); // This is expressed in longs

    // Enable the screen with the video mode
    R32(0xFC000000) = 1 + (MODEREZ << 1);
#endif
}

static WORD get_monitor_type(void)
{
    KDEBUG(("screen_1bpp:get_monitor_type\n"));
    return MON_MONO; /* TODO: Would VGA be more accurate (be it for the 60Hzfrequency rather than 72Hz) */
}

static WORD get_number_of_colors_nuances(void)
{
    KDEBUG(("screen_1bpp:get_number_of_colors_nuances\n"));
    return 2;
}

static UBYTE *vicky3_physbase(void)
{
#if 0
    KDEBUG(("screen_1bpp:vicky3_physbase\n"));
    return (UBYTE *)(R32(VICKY3+0x4)*4);
#else
    return 0x200000;
#endif
}

static WORD vicky3_setscreen(UBYTE *logical, const UBYTE *physical, WORD rez, WORD videlmode)
{
    KDEBUG(("screen_1bpp:vicky3_setscreen\n"));
    return MODEREZ;
}

static void set_palette(const UWORD *new_palette)
{
    KDEBUG(("screen_1bpp:set_palette\n"));
    // TODO
}

const SCREEN_DRIVER a2560_screen_driver_1bpp = {
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
