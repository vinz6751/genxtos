/*
 * biosvecs.i - exports from from biosvecs.S 
 *
 * Copyright (C) 2001-2021 The EmuTOS development team
 *
 * Authors:
 *  VB     Vincent Barrilliot
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */

#ifndef BIOSVECS_I
#define BIOSVECS_I
        .extern _halt
        .extern _kill_program
        .extern _warm_reset
        .extern _cold_reset
#endif
