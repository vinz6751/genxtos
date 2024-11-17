/* Definitions for the MFP68901 chip
 * Copyright (C) 2001 Martin Doering
 * Copyright (C) 2001-2022 The EmuTOS development team
 *
 * Authors:
 *  MAD     Martin Doering
 *  LVL     Laurent Vogel
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#ifndef MFP68901_H
#define MFP68901_H

#include "portab.h"

/*==== MFP memory mapping =================================================*/
typedef struct
{
    UBYTE   dum1;
    volatile UBYTE  gpip;   /* general purpose .. register */
    UBYTE   dum2;
    volatile UBYTE  aer;    /* active edge register              */
    UBYTE   dum3;
    volatile UBYTE  ddr;    /* data direction register           */
    UBYTE   dum4;
    volatile UBYTE  iera;   /* interrupt enable register A       */
    UBYTE   dum5;
    volatile UBYTE  ierb;   /* interrupt enable register B       */
    UBYTE   dum6;
    volatile UBYTE  ipra;   /* interrupt pending register A      */
    UBYTE   dum7;
    volatile UBYTE  iprb;   /* interrupt pending register B      */
    UBYTE   dum8;
    volatile UBYTE  isra;   /* interrupt in-service register A   */
    UBYTE   dum9;
    volatile UBYTE  isrb;   /* interrupt in-service register B   */
    UBYTE   dum10;
    volatile UBYTE  imra;   /* interrupt mask register A         */
    UBYTE   dum11;
    volatile UBYTE  imrb;   /* interrupt mask register B         */
    UBYTE   dum12;
    volatile UBYTE  vr;     /* vector register                   */
    UBYTE   dum13;
    volatile UBYTE  tacr;   /* timer A control register          */
    UBYTE   dum14;
    volatile UBYTE  tbcr;   /* timer B control register          */
    UBYTE   dum15;
    volatile UBYTE  tcdcr;  /* timer C + D control register      */
    UBYTE   dum16;
    volatile UBYTE  tadr;   /* timer A data register             */
    UBYTE   dum17;
    volatile UBYTE  tbdr;   /* timer B data register             */
    UBYTE   dum18;
    volatile UBYTE  tcdr;   /* timer C data register             */
    UBYTE   dum19;
    volatile UBYTE  tddr;   /* timer D data register             */
    UBYTE   dum20;
    volatile UBYTE  scr;    /* synchronous character register    */
    UBYTE   dum21;
    volatile UBYTE  ucr;    /* USART control register            */
    UBYTE   dum22;
    volatile UBYTE  rsr;    /* receiver status register          */
    UBYTE   dum23;
    volatile UBYTE  tsr;    /* transmitter status register       */
    UBYTE   dum24;
    volatile UBYTE  udr;    /* USART data register               */
} MFP;

void mfp68901_reset_regs(MFP *mfp);
void mfp68901_disable_interrupt(MFP *mfp, WORD num);
void mfp68901_enable_interrupt(MFP *mfp, WORD num);

#endif
