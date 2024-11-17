
#include "config.h"
#if CONF_WITH_FDC
# include "floppy.h"
#endif
#include "portab.h"
#if CONF_WITH_ATARI_VIDEO
# include "screen.h"
#endif
#include "tosvars.h"
#include "vt52.h"

// These are separate functions for clarity, but GCC will inline them.
static void dump_screen(void);
static void copy_palette(void);
static void set_new_screen_address(void);
static void process_vbl_queue(void);;


void vbl_handler(void);

// C-side part of the VBL handling called from int_vbl
void vbl_handler(void) {

#if CONF_WITH_ATARI_VIDEO
    // check for monitor switching and jump to _swv_vec if necessary...
    detect_monitor_change();
#endif

#if !(defined(MACHINE_A2560U) || defined(MACHINE_A2560X)) || CONF_WITH_A2560U_SHADOW_FRAMEBUFFER /* If we have text mode only, VICKY takes care of the blinking */
    // blink cursor
    vt52_blink();
#endif

#if CONF_WITH_A2560U_SHADOW_FRAMEBUFFER
    if (!a2560u_bios_sfb_is_active) {
        a2560u_sfb_addr = v_bas_ad;
        a2560u_sfb_copy_fb_to_vram();
    }
#endif

    copy_palette();
    set_new_screen_address();

#if CONF_WITH_FDC
    flopvbl();
#endif

    process_vbl_queue();

    dump_screen();
}


static void copy_palette(void) {
    // Support of Setpalette
    if (colorptr) {
#if (defined(MACHINE_A2560U) || defined(MACHINE_A2560X)) && 0
        WORD i;
        *VICKY_B_BG_COLOR = colorptr[0];
        for (i=0; i<16; i++) {
            uint32_t color = convert_atari2vicky_color(colorptr[i]);
            if (i == 0) {
                *VICKY_B_BG_COLOR = color;
            }
            else {
                vicky2_set_lut_color(vicky, 0, i, color);
            }
        }
#elif CONF_WITH_ATARI_VIDEO
        // the contents of colorptr indicate the palette processing required; since
        // the source address must be on a word boundary, we use bit 0 as a flag:
        //  contents                 meaning
        //  --------                 -------
        //     0                     do nothing
        //  address                  load ST(e) palette registers from address
        //  address with flag set    load 16 Falcon palette registers from address
        //     0 with flag set       load 256 Falcon palette registers from
        //                             _falcon_shadow_palette
        ULONG *palette; // Use longs so it's (hopefully?) faster to copy
        WORD palette_size;

        if ((LONG)colorptr & 1) {
            colorptr = (UWORD*)((LONG)colorptr & ~1); // Test & clear Falcon indicator
#if CONF_WITH_VIDEL
            palette = (ULONG*)0xffff9800L;
            palette_size = falcon_shadow_count - 1;
#endif
        }
        else {
            palette = (ULONG*)0xffff8240L;
            palette_size = 16/2 -1; // Number of colors of the Shifter / 2 we copy LONGS. Should we care about the actual number of colors the resolution allows ?
        }
        // Copy the palette
        do {
            *palette++ = *colorptr++;
        } while (--palette_size >= 0); // Hope for dbra
#endif
        colorptr = 0; // Job done
    }
}


static void set_new_screen_address(void) {
    // Support of set new video address at VBL (_screenpt variable)
    if (screenpt) {
        screen_setphys(screenpt);
    }
}


// Using volatile memory means we don't care if a VBL queue routine clobbers registers used here
static volatile WORD i_vbl_queue;

static void process_vbl_queue(void) {
    // Process VBL queue, take semaphore
    for (i_vbl_queue=0; i_vbl_queue<nvbls; i_vbl_queue++) {
        PFVOID routine = vblqueue[i_vbl_queue];
        if (routine) {
            // TODO ! SAVE/RESTORE REGISTERS
            routine();
        }
    }
}


static void dump_screen(void) {
    // Dump screen if requested
    if (!dumpflg) {
        dump_vec();
        dumpflg = -1;
    }
}