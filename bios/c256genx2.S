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

#define ASM_ONLY
#include "c256genx.h"

	.global _PS2_send_controller

	SECTION_RODATA

_PS2_Init:
	ori.b	#FNX1_INT00_KBD, INT_MASK_REG1

// Send a byte to the PS2 controller
_PS2_send_controller:
	jbsr	PS2_wait_ctrl_ready
	move.b	4(sp),PS2_wait_ctrl_ready
	rts

// Wait for the PS/2 controller to be ready to receive a command byte
PS2_wait_ctrl_ready:
//	move.b	PS2_KCTRL,d0
//	andi.b	#PS2_STATUS_OUPUT,d0
	bne.s   PS2_wait_ctrl_ready
	rts

#endif