/*
 * console.h - console header
 *
 * Copyright (C) 2001-2019 The EmuTOS development team
 *
 * Authors:
 *  LVL   Laurent Vogel
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#ifndef CONSOLE_H
#define CONSOLE_H

#include "portab.h"

/******************************************
**
** BDOS level character device file handles
**
*******************************************
*/

#define H_Console       -1
#define H_Aux           -2
#define H_Print         -3


/****************************************
**
** Character device handle conversion
** (BDOS-type handle to BIOS-type handle)
**
*****************************************
*/

#define HXFORM(h)       (3+h)



void stdhdl_init(void);
signed char get_default_handle(int stdh);

long GEMDOS_API xconstat(void);
long GEMDOS_API xconostat(void);
long GEMDOS_API xprtostat(void);
long GEMDOS_API xauxistat(void);
long GEMDOS_API xauxostat(void);
long GEMDOS_API xconout(int ch);
long GEMDOS_API xauxout(int ch);
long GEMDOS_API xprtout(int ch);
long GEMDOS_API xrawcin(void);
long GEMDOS_API xconin(void);
long GEMDOS_API xnecin(void);
long GEMDOS_API xauxin(void);
long GEMDOS_API xrawio(int parm);
void GEMDOS_API xconws(char *p);
void GEMDOS_API xconrs(char *p);
int cgets(int h, int maxlen, char *buf);
long conin(int h);
void tabout(int h, int ch);



#endif /* CONSOLE_H */
