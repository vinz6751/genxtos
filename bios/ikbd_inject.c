/*
 * ikbd_inject.c - Inject IKBD packets into ikbdraw for FASTCALL/ABI tests
 */

#include "emutos.h"

#ifndef NATFEAT_DEBUG

/* Keep a non-empty translation unit when NATFEAT_DEBUG=0. */
static void ikbd_inject_unused(void) {}

#else

#include "aciavecs.h"
#include "asm.h"
#include "ikbd.h"
#include "ikbd_inject.h"
#include "kprint.h"
#include "xbiosbind.h"

/* Desk runs in user mode; ikbdraw must run in supervisor like ACIA IRQ. */
static UBYTE inj_byte;

static LONG inject_byte_super(void)
{
    WORD sr = set_sr(0x2700);
    ikbdraw(inj_byte);
    set_sr(sr);
    return 0;
}

void ikbd_inject_byte(UBYTE b)
{
    inj_byte = b;
    Supexec((LONG)inject_byte_super);
}

void ikbd_inject_mouse_rel(UBYTE hdr, SBYTE dx, SBYTE dy)
{
    NF_TRACE(("IKBD-INJECT mouse hdr=0x%02x dx=%d dy=%d\n",
              (unsigned)hdr, (int)dx, (int)dy));
    ikbd_inject_byte(hdr);
    ikbd_inject_byte((UBYTE)dx);
    ikbd_inject_byte((UBYTE)dy);
}

void ikbd_inject_key(UBYTE scancode)
{
    NF_TRACE(("IKBD-INJECT key 0x%02x\n", (unsigned)scancode));
    ikbd_inject_byte(scancode);
}

void ikbd_inject_selftest(void)
{
    /*
     * Default: left down/up (desktop click path).
     * Override: -DIKBD_INJECT_MODE=0 move-only, 1 left click, 2 full,
     * 3 menu-bar hover (relative moves into a title).
     */
#ifndef IKBD_INJECT_MODE
#define IKBD_INJECT_MODE 1
#endif
    UBYTE left_down = (UBYTE)(MOUSE_REL_POS_REPORT | LEFT_BUTTON_DOWN);
    UBYTE right_down = (UBYTE)(MOUSE_REL_POS_REPORT | RIGHT_BUTTON_DOWN);
    UBYTE none = MOUSE_REL_POS_REPORT;
    int i;

    NF_TRACE(("IKBD-INJECT selftest begin mode=%d\n", IKBD_INJECT_MODE));

#if IKBD_INJECT_MODE == 0
    ikbd_inject_mouse_rel(none, 1, 0);
    NF_TRACE(("IKBD-INJECT after move\n"));
#elif IKBD_INJECT_MODE == 1
    ikbd_inject_mouse_rel(left_down, 0, 0);
    NF_TRACE(("IKBD-INJECT after left down\n"));
    ikbd_inject_mouse_rel(none, 0, 0);
    NF_TRACE(("IKBD-INJECT after left up\n"));
#elif IKBD_INJECT_MODE == 3
    /*
     * Relative moves into a menu title (THEACTIVE), not empty bar.
     * Stay under NFORKS (32).  Mid-screen x is often past the titles;
     * go up then left toward Desk/File.
     */
    for (i = 0; i < 8; i++)
        ikbd_inject_mouse_rel(none, 0, (SBYTE)-24);
    NF_TRACE(("IKBD-INJECT after move-to-menubar\n"));
    for (i = 0; i < 6; i++)
        ikbd_inject_mouse_rel(none, (SBYTE)-40, 0);
    NF_TRACE(("IKBD-INJECT after move-to-title\n"));
#else
    ikbd_inject_mouse_rel(none, 1, 0);
    NF_TRACE(("IKBD-INJECT after move\n"));
    ikbd_inject_mouse_rel(left_down, 0, 0);
    NF_TRACE(("IKBD-INJECT after left down\n"));
    ikbd_inject_mouse_rel(none, 0, 0);
    NF_TRACE(("IKBD-INJECT after left up\n"));
    ikbd_inject_mouse_rel(right_down, 0, 0);
    NF_TRACE(("IKBD-INJECT after right down\n"));
    ikbd_inject_mouse_rel(none, 0, 0);
    NF_TRACE(("IKBD-INJECT after right up\n"));
#endif
    (void)left_down;
    (void)right_down;
    (void)none;
    (void)i;

    NF_TRACE(("IKBD-INJECT selftest end\n"));
}

#endif /* NATFEAT_DEBUG */
