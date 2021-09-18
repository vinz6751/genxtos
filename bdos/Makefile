TARGET = bdos.tos
.DEFAULT_GOAL = $(TARGET)


include ../Makefile.inc


INCDIR += -I../include
LDPATH += -L.. -L../linker 

SRC := bdosmain.o console.o fsbuf.o fsdir.o fsdrive.o fsfat.o fsglob.o fshand.o fsio.o fsmain.o fsopnclo.o iumem.o kpgmld.o osmem.o umem.o proc.o rwa.o	time.o bdostst.o ../util/libutil.a


bdos.tos: $(OBJ) bdos.ld Makefile bdosvars.ld
#../bios/biosvars.ld
#../linker/section_first_stram.ld ../bios/biosvars.ld
	$(LD) -o $@ $(LDPATH) -Wl,-T,bdos.ld $(OBJ)
#-Wl,-T,../bios/biosvars.ld 

../util/libutil.a:
	cd ../util && $(MAKE) 

bdos.ld: bdos.ld_
	cd .. && $(MAKE) -f Makefile.ld

