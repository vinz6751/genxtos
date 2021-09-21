/*
 * initinfo.h - Info screen at startup
 *
 * Copyright (C) 2001-2021 by Authors:
 *
 * Authors:
 *  MAD     Martin Doering
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */



#ifndef INITINFO_H
#define INITINFO_H

/*==== Prototypes =========================================================*/

void initinfo_show(struct boot_settings *settings);
void initinfo_show_boot_msg(void);

#endif /* INITINFO_H */
