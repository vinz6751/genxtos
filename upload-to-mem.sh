#!/bin/sh
FNXMGR="python3 $FOENIXMGR/FoenixMgr/fnxmgr.py --port /dev/ttyUSB1"
$FNXMGR --upload-srec ../EmuTOS-RAM-loader/ldemutos.s28 ; $FNXMGR --binary emutos-a2560m.rom --address 0x100000
