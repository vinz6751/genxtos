# change "define ROM_ORIGIN 0x00e00000" if needed
make clean;make -f Makefile.ld clean
make a2560u UNIQUE=fr
make -f Makefile.ld
make a2560u UNIQUE=fr
then use "upload-to-flash" script
