#!/bin/bash
# Convenience script for building
pushd foenix
rm libfoenix.a
make -j4
popd
rm -f emutos.img emutos-a2560m.rom
# Change "fr" to your country code and 4 to the number of CPU cores you have
make UNIQUE=fr a2560m -j4
