/*
 * time.h - GEMDOS time and date functions
 *
 * Copyright (C) 2002-2019 the EmuTOS development team
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#ifndef TIME_H
#define TIME_H

#include "portab.h"

long GEMDOS_API xgetdate(void);
long GEMDOS_API xsetdate(UWORD d);
long GEMDOS_API xgettime(void);
long GEMDOS_API xsettime(UWORD t);

/* called to initialize the module */
void time_init(void);

#endif /* TIME_H */
