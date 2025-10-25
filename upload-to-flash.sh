#!/bin/sh
COM=/dev/ttyUSB1
FNXMGR="python3 $FOENIXMGR/FoenixMgr/fnxmgr.py --port $COM"
ROM=`ls emutos-a2560?.rom`
echo "Uploading $ROM to flash through port $COM"
$FNXMGR --flash $ROM --address 0x000000
