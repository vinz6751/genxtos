/* m68k trap handler for the trap interface of the library.
 * This just catches the trap and calls the trap_dispatch in C, with the arguments on the stack.
 * Author: Vincent Barrilliot
 * tabSize=4
 */

#include "config.h"
	
	.GLOBL	_trap_irq_handler
	.EXTERN	_cpu_has_long_frames
	
_trap_irq_handler:
	movem.l	d1/a0-a1,-(sp) /* Save registers */
	btst	#5,0xc(sp) /* Are we called from supervisor mode ? 0xc because skip 12 bytes from 3 saved registers, and examinig MSB of sr */
	beq.s	not_called_from_supervisor
called_from_supervisor:
#if LONG_STACK_FRAMES_SUPPORT
	tst.w	_cpu_has_long_frames
	beq.s	short_frame
	lea		20(sp),a0 /* Skip the sr (2), format/vector word (2), pc(4) and 3 register saves (3*4) */
	bra.s	call_dispatch
#endif /* LONG_STACK_FRAMES_SUPPORT */
short_frame:
	lea		18(sp),a0 /* Skip the sr (2), pc(4) and 3 register saves (3*4) */
	bra.s	call_dispatch
not_called_from_supervisor:
	move.l	usp,a0
call_dispatch:
	move.l	a0,-(sp)
	jsr		_trap_dispatch
	addq.l	#4,sp
restore:
	movem.l	(sp)+,d1/a0-a1 /* Restore registers */
	rte
