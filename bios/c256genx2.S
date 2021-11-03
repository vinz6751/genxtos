/*
 * c256genx.S - C256 Foenix GenX specific assembler functions
 *
 * Copyright (C) 2013-2021 The EmuTOS development team
 *
 * Authors:
 *  VB   Vincent Barrilliot
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */


#ifdef MACHINE_C256FOENIXGENX

#include "asmdefs.h"

        .globl  c256foenixgenx_startup // First thing called by the BIOS on boot

// a6 contains the return address
c256foenixgenx_startup:
	jmp	(a6)

#endif