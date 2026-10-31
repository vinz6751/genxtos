/*
 * midi.h - header for MIDI routines
 *
 * Copyright (C) 2001-2024 The EmuTOS development team
 *
 * Authors:
 *  LVL   Laurent Vogel
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#ifndef MIDI_H
#define MIDI_H

/* initialise the MIDI ACIA */
void midi_init(void);

/* some bios functions */
LONG ARGS_ON_STACK bconstat3(void);
LONG ARGS_ON_STACK bconin3(void);
LONG ARGS_ON_STACK bcostat3(void);
LONG ARGS_ON_STACK bconout3(WORD dev, WORD c);

/* some xbios functions */
void ARGS_ON_STACK midiws(WORD cnt, const UBYTE *ptr);

#endif /* MIDI_H */
