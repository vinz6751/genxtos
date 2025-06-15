#ifndef ACIAVECS_H
#define ACIAVECS_H

#include "emutos.h"
#include "iorec.h"

void init_acia_vecs(void);

extern void (*kbdvec)(UBYTE data, IOREC *iorec_a0); /* KBD Input, TOS >= 2.0, must be before must be right before kbdvecs */

/* the following is in aciavecs.S */
void call_mousevec(SBYTE *packet);
#ifdef MACHINE_AMIGA
void call_joyvec(UBYTE *packet);
#endif

// Inlet from bytes (supposedly or not) received from the IKBD
void ikbdraw(UBYTE b);
// Use C implementation of ikbdraw
#define call_ikbdraw(x) ikbdraw(x)

 #endif