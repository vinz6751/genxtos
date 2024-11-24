/*
 * cartridge_atari.S - Atari cartridge port supporting functions
 *
 * Copyright (C) 2001-2024 The EmuTOS development team
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#ifndef CARTRIDGE_ATARI_H
#define CARTRIDGE_ATARI_H


#define ATARI_CARTRIDGE_BASE 0x00fa0000L // Base of ROM port cartridge

/* Runs an bootable cartridge */
void cartridge_atari_boot(void);

#endif
