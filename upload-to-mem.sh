#!/bin/sh
fnxmgr --upload-srec ../EmuTOS-RAM-loader/ldemutos.s28 ; fnxmgr --binary emutos-a2560u.rom --address 0x100000
