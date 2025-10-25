#!/bin/sh
FNXMGR="python3 $FOENIXMGR/FoenixMgr/fnxmgr.py --port /dev/ttyUSB1"
fnxmgr --flash emutos-a2560m.rom --address 0x000000
