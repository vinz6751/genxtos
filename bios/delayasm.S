/*
 * delayasm.S - assembler functions for delay calibration
 *
 * Copyright (C) 2013-2022 The EmuTOS development team
 *
 * Authors:
 *  RFB   Roger Burrows
 *        Christian Zietz
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#include "config.h"

#if CONF_WITH_MFP

#include "asmdefs.h"

        .globl  _run_calibration

#define TIMERD_VEC  0x110
#define MFP_REGS    0xfffffa01
#define MFP_IERB    MFP_REGS+8
#define MFP_ISRB    MFP_REGS+16
#define MFP_IMRB    MFP_REGS+20
#define MFP_TCDCR   MFP_REGS+28
#define MFP_TDDR    MFP_REGS+36

        .text
//
// calibrate using MFP TimerD
//
_run_calibration:
        clr.l   intcount            //initialise interrupt count

        lea     MFP_TCDCR,a0        //a0 -> Timer C/D control reg
#ifdef __mcoldfire__
        move.b  (a0),d0
        andi.l  #0xf0,d0            //stop TimerD
        move.b  d0,(a0)
#else
        andi.b  #0xf0,(a0)          //stop TimerD
#endif

        lea     MFP_TDDR,a0         //a0 -> TimerD data
        clr.b   (a0)                //set count = 256
        move.l  TIMERD_VEC,a1       //save old TimerD vector in a1
        lea     handle_timer,a0
        move.l  a0,TIMERD_VEC       //set up new TimerD vector
        lea     MFP_IERB,a0
        bset.b  #4,(a0)             //set interrupt enable bit
        lea     MFP_IMRB,a0
        bset.b  #4,(a0)             //set interrupt mask bit

        lea     MFP_TCDCR,a0        //a0 -> Timer C/D control reg
#ifdef __mcoldfire__
        move.b  (a0),d0
        ori.l   #0x02,d0            //set pre-scaler = divide-by-10 & start TimerD
        move.b  d0,(a0)
#else
        ori.b   #0x02,(a0)          //set pre-scaler = divide-by-10 & start TimerD
#endif

        move.l  4(a7),d0            //loopcount
cal_loop:                           //just like asm.h inline function
        subq.l  #1,d0
        jpl     cal_loop

        move.l  intcount,d0         //d0 = interrupt count
        move.l  d0,intsave          //save it

        move    sr,d1
        move    #0x2700,sr          //disable interrupts to avoid spurious irq

        lea     MFP_IERB,a0
        bclr.b  #4,(a0)             //reset interrupt enable bit
        lea     MFP_IMRB,a0
        bclr.b  #4,(a0)             //reset interrupt mask bit

        lea     MFP_TCDCR,a0        //a0 -> Timer C/D control reg
#ifdef __mcoldfire__
        move.b  (a0),d0
        andi.l  #0xf0,d0            //stop TimerD
        move.b  d0,(a0)
#else
        andi.b  #0xf0,(a0)          //stop TimerD
#endif

        move    d1,sr               //restore interrupt mask

        move.l  a1,TIMERD_VEC       //restore TimerD vector
        move.l  intsave,d0          //return interrupt count
        rts

//
// handle timer interrupt from calibration
//
// an interrupt occurs every 256/(2457600/10) seconds,
// i.e. every 1/960 second.
//
// overhead calculation
// --------------------
//   68000 at 8MHz
//       an interrupt occurs every 8000000/960 = 8333 cycles.
//       the overhead is 112/8333, approx 1.4%
//   other systems are even less, so we ignore
//
handle_timer:                   //68000:
        //interrupt dispatch      //44
        addq.l  #1,intcount       //12+16 = 28
#ifdef __mcoldfire__
        move.l  a0,-(sp)
        lea     MFP_ISRB,a0
        move.b  #0xef,(a0)
        movea.l (sp)+,a0
#else
        move.b  #0xef,MFP_ISRB    //20
#endif
        rte                       //20
                                //total 112
        .bss
intcount: .ds.l 1
intsave:  .ds.l 1

#endif /* CONF_WITH_MFP */