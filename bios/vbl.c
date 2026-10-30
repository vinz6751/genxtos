
#include "config.h"
#include "conout.h"
#if CONF_WITH_FDC
# include "floppy.h"
#endif
#include "portab.h"
//#if CONF_WITH_ATARI_VIDEO // Some of the stuff is probably Atari specific
# include "screen.h"
//#endif
#include "tosvars.h"
#include "vbl.h"
#include "videl.h"

#if CONF_WITH_A2560_SHADOW_FRAMEBUFFER
#include "a2560_bios.h"
#include "../foenix/shadow_fb.h"
#endif

extern PFVOID vbl_list[8]; /* Default array for the vblqueue TOS variable */

// These are separate functions for clarity, but GCC will inline them.
static void dump_screen(void);
static void process_vbl_queue(void);

// Doesn't need to be shared with anything (i.e put in a .h file), that's only called from vectors.S
void vbl_handler(void);


// Initialise the VBL stuff
void vbl_init(void) {
    int i;
    nvbls = ARRAY_SIZE(vbl_list);
    vblqueue = vbl_list;
    for(i = 0 ; i < nvbls ; i++) {
        vbl_list[i] = NULL;
    }
}


// C-side part of the VBL handling called from int_vbl
void vbl_handler(void) {

#if CONF_WITH_ATARI_VIDEO
    // check for monitor switching and jump to _swv_vec if necessary...
    screen_detect_monitor_change();
#endif

#if !(defined(MACHINE_A2560U) || defined(MACHINE_A2560K) || defined(MACHINE_A2560M) || defined(MACHINE_A2560X) || defined(MACHINE_GENX)) || CONF_WITH_A2560_SHADOW_FRAMEBUFFER /* If we have text mode only, VICKY takes care of the blinking */
    // blink cursor
    conout_blink_cursor();
#endif

#if CONF_WITH_A2560_SHADOW_FRAMEBUFFER && 0 /* Disabled because I'm trying something different (write both to SRAM and VRAM )*/
    if (!a2560_bios_sfb_is_active) {
        a2560_sfb_addr = v_bas_ad;
        a2560_sfb_copy_fb_to_vram();
    }
#endif

    // Support of Setpalette
    if (colorptr) {
        screen_do_set_palette((const UWORD*)colorptr);
        colorptr = 0;
    }

    // Support of set new video address at VBL (_screenpt variable)
    if (screenpt) {
        screen_setphys(screenpt);
        screenpt = 0;
    }

#if CONF_WITH_FDC
    flopvbl();
#endif

    process_vbl_queue();

    dump_screen();
}


// Using volatile memory means we don't care if a VBL queue routine clobbers registers used here
static volatile WORD i_vbl_queue;

static void process_vbl_queue(void) {
    // Process VBL queue, take semaphore
    for (i_vbl_queue=0; i_vbl_queue<nvbls; i_vbl_queue++) {
        PFVOID routine = vblqueue[i_vbl_queue];
        if (routine) {
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
