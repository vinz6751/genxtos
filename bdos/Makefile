TARGET = bdos.tos
.DEFAULT_GOAL = $(TARGET)


include ../Makefile.inc


INCDIR += -I../include
LDPATH += -L.. -L../linker 

SRC := bdosmain.o console.o fsbuf.o fsdir.o fsdrive.o fsfat.o fsglob.o fshand.o fsio.o fsmain.o fsopnclo.o iumem.o kpgmld.o osmem.o umem.o proc.o rwa.o	time.o bdostst.o ../util/libutil.a


bdos.tos: $(OBJ) bdos.ld Makefile bdosvars.ld
	$(LD) -o $@ $(LDPATH) -Wl,-T,bdos.ld $(OBJ)


../util/libutil.a:
	cd ../util && $(MAKE) 

