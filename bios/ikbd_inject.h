/*
 * ikbd_inject.h - Test harness to feed IKBD bytes into ikbdraw / KBDVECS
 *
 * Built only with CONF_WITH_IKBD_INJECT=1 (auto-on when NATFEAT_DEBUG=1).
 * Injects at the same point as the ACIA ikbdsys path (ikbdraw), so
 * mousevec / user_but / user_mot and keyboard kbdvec all see realistic
 * packets.
 */

#ifndef IKBD_INJECT_H
#define IKBD_INJECT_H

#include "portab.h"
#include "config.h"

#if CONF_WITH_IKBD_INJECT

#include "ikbd.h"   /* MOUSE_REL_POS_REPORT, LEFT/RIGHT_BUTTON_DOWN */

/* Feed one raw IKBD byte (same as ACIA → ikbdsys → ikbdraw). */
void ikbd_inject_byte(UBYTE b);

/* Absolute convenience: relative mouse report header + dx + dy. */
void ikbd_inject_mouse_rel(UBYTE hdr, SBYTE dx, SBYTE dy);

/* Make-code / break-code scancode (single-byte key event). */
void ikbd_inject_key(UBYTE scancode);

/*
 * Self-test sequence used by the desktop main loop under inject builds:
 *   1) relative move
 *   2) AES-left button down/up  (ikbd.h LEFT_BUTTON_DOWN → 0xfa)
 *   3) AES-right button down/up (ikbd.h RIGHT_BUTTON_DOWN → 0xf9)
 * Logs IKBD-INJECT milestones via NF_TRACE.
 */
void ikbd_inject_selftest(void);

#endif /* CONF_WITH_IKBD_INJECT */

#endif /* IKBD_INJECT_H */
