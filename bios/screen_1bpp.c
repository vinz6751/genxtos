
#define ENABLE_KDEBUG 1

#include "emutos.h"

#if defined(MACHINE_A2560K) || defined(MACHINE_A2560X) || defined(MACHINE_GENX)

#include "biosdefs.h" /* MON_MONO */
#include "biosext.h"  /* EXTRA_VRAM_SIZE */
#include "screen.h"
#include "tosvars.h"  /* v_bas_ad */
#include "vectors.h"  /* int_vbl */
#include "../foenix/cpu.h"
#include "../foenix/interrupts.h"
#include "../foenix/regutils.h"
#include "../foenix/vicky2.h"

/*
 * Foenix MCP documents the A2560K/X/GenX 1bpp boot mode as VICKY B TOS graph
 * scanout from VRAM bank 0 with colors in TOSGRAPH[4] and [5].
 */
#define MODEREZ 4 /* EmuTOS mono mode; height is 960 or 1024 from the HiRes DIP */
#define TOSGRAPH_WIDTH 1280
#define TOSGRAPH_PALETTE_SIZE 2

/* IRQ handlers */
void a2560_irq_vicky_a(void); /* VICKY autovector A interrupt handler */
void a2560_irq_vicky_b(void); /* VICKY autovector B interrupt handler */

static ULONG calc_vram_size(void);

static UWORD tosgraph_palette[TOSGRAPH_PALETTE_SIZE] = {
    RGB_WHITE, RGB_BLACK,
};


static UWORD get_height(void)
{
    return (R32(VICKY_B) & VICKY_CTRL_HIGH_RES) ? 1024 : 960;
}

static void apply_resolution_bits(void)
{
    ULONG ctrl = R32(VICKY_B);

    if (ctrl & VICKY_CTRL_HIGH_RES)
        ctrl |= VICKY_B_CTRL_MODE0;
    else
        ctrl &= ~((ULONG)VICKY_B_CTRL_MODE0);

    R32(VICKY_B) = ctrl;
}

static void apply_tosgraph_palette(void)
{
    R32(VICKY_B_TOSGRAPH_COLOR0) = convert_atari2vicky_color(tosgraph_palette[0]);
    R32(VICKY_B_TOSGRAPH_COLOR1) = convert_atari2vicky_color(tosgraph_palette[1]);
}

static void tosgraph_disable(void)
{
    R32(VICKY_B_TOSGRAPH_MODE) = 0;
    R32(VICKY_B) &= ~((ULONG)VICKY_B_CTRL_TOS_GRAPH);
}

static void init(void)
{
    KDEBUG(("screen_1bpp:init\n"));

    a2560_irq_set_handler(INT_SOF_B, int_vbl);

    cpu_set_vector(INT_VICKYII_A, (uint32_t)a2560_irq_vicky_a);
    cpu_set_vector(INT_VICKYII_B, (uint32_t)a2560_irq_vicky_b);


    apply_resolution_bits();
    apply_tosgraph_palette();
    tosgraph_disable();
}

static ULONG calc_vram_size(void)
{
    ULONG size = (ULONG)(TOSGRAPH_WIDTH / 8) * (ULONG)get_height() + EXTRA_VRAM_SIZE;

    KDEBUG(("screen_1bpp:calc_vram_size returning %ld\n", size));
    return size;
}

static WORD screen_1bpp_check_moderez(WORD moderez)
{
    MAYBE_UNUSED(moderez);
    KDEBUG(("screen_1bpp:check_moderez\n"));
    return MODEREZ;
}

static void screen_1bpp_initialise_palette_registers(WORD rez, WORD mode)
{
    MAYBE_UNUSED(rez);
    MAYBE_UNUSED(mode);
    KDEBUG(("screen_1bpp:initialise_palette_registers\n"));
    apply_tosgraph_palette();
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
    *width = TOSGRAPH_WIDTH;
    *height = get_height();
}

static void setphys(const UBYTE *addr)
{
    ULONG ctrl;

    KDEBUG(("screen_1bpp:setphys(%p)\n", addr));

    if (addr && (addr != (const UBYTE *)VRAM_Bank0)) {
        KDEBUG(("screen_1bpp:setphys rejecting unsupported addr %p\n", addr));
        return;
    }

    apply_resolution_bits();
    apply_tosgraph_palette();

    ctrl = R32(VICKY_B);
    ctrl &= ~(VICKY_CTRL_TXTOVERLAY | VICKY_CTRL_GFX | VICKY_CTRL_BITMAP);
    ctrl |= (ULONG)(VICKY_B_CTRL_TOS_GRAPH | VICKY_CTRL_TEXT);
    R32(VICKY_B) = ctrl;
    R32(VICKY_B_TOSGRAPH_MODE) = VICKY_B_TOSGRAPH_1BPP;
}

static WORD screen_1bpp_get_monitor_type(void)
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
    KDEBUG(("screen_1bpp:vicky3_physbase\n"));
    return (UBYTE *)VRAM_Bank0;
}

static WORD vicky3_setscreen(UBYTE *logical, const UBYTE *physical, WORD rez, WORD videlmode)
{
    WORD oldmode = 0;

    MAYBE_UNUSED(videlmode);
    KDEBUG(("screen_1bpp:vicky3_setscreen(logical=%p, physical=%p, rez=%d, videlmode=%d)\n",
        logical, physical, rez, videlmode));

    if ((LONG)logical > 0) {
        if (logical != (UBYTE *)VRAM_Bank0) {
            KDEBUG(("screen_1bpp:unsupported logical screen %p\n", logical));
            return -1;
        }
        v_bas_ad = logical;
    }

    if ((LONG)physical > 0) {
        if (physical != (const UBYTE *)VRAM_Bank0) {
            KDEBUG(("screen_1bpp:unsupported physical screen %p\n", physical));
            return -1;
        }
        screen_setphys(physical);
    }

    if (rez_was_hacked || (rez == -1))
        return oldmode;

    if ((rez < MIN_REZ) || (rez > MAX_REZ)) {
        KDEBUG(("screen_1bpp:invalid rez = %d\n", rez));
        return -1;
    }

    if (rez != MODEREZ) {
        KDEBUG(("screen_1bpp:unsupported rez = %d\n", rez));
        return -1;
    }

    vsync();
    apply_resolution_bits();
    screen_init_services_from_mode_info();

    return oldmode;
}

static WORD screen_1bpp_setcolor(WORD colorNum, WORD color)
{
    WORD oldcolor;

    if ((colorNum < 0) || (colorNum >= TOSGRAPH_PALETTE_SIZE))
        return -1;

    oldcolor = tosgraph_palette[colorNum];
    tosgraph_palette[colorNum] = color;

    if (colorNum < 2)
        apply_tosgraph_palette();

    return oldcolor;
}

static void set_palette(const UWORD *new_palette)
{
    WORD i;

    KDEBUG(("screen_1bpp:set_palette\n"));
    for (i = 0; i < TOSGRAPH_PALETTE_SIZE; i++)
        tosgraph_palette[i] = new_palette[i];

    apply_tosgraph_palette();
}

const SCREEN_DRIVER a2560_screen_driver_1bpp = {
    init,
    calc_vram_size,
    screen_1bpp_check_moderez,
    screen_1bpp_initialise_palette_registers,
    can_change_resolution,
    get_current_mode_info,
    setphys,
    screen_1bpp_get_monitor_type,
    get_number_of_colors_nuances,
    get_std_pixel_size,
    vicky3_physbase,
    vicky3_setscreen,
    screen_1bpp_setcolor,
    set_palette,
};

#endif
