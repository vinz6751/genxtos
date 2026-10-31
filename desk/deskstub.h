/*
 * deskstub.h - EmuDesk entry point, called by AES
 *
 * Copyright (C) 2019 The EmuTOS development team
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#ifndef _DESKSTUB_H
#define _DESKSTUB_H

void deskstart(void) NORETURN;   /* see deskstart.S */
void desk_run_inprocess(void);   /* in-process desktop wrapper */
void desk_boot_enter(void);      /* NATFEAT_DEBUG milestone before deskmain */

#endif /* _DESKSTUB_H */
