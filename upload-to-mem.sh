#!/bin/sh
COM=/dev/ttyUSB1
FNXMGR="python3 $FOENIXMGR/FoenixMgr/fnxmgr.py --port $COM"
ROM=`ls emutos-a2560?.rom`
echo "Uploading $ROM to memory through port $COM"
#ldemutos.s28 is a simple program that sets the reset vector to 0x100000. There
#are probably other/better ways of doing that.
$FNXMGR --upload-srec tools/ldemutos.s28 ; $FNXMGR --binary $ROM --address 0x100000
