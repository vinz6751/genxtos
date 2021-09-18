# Compilation toolchain
TOOLCHAIN=m68k-atari-mint-
CC=$(TOOLCHAIN)gcc
AS=$(TOOLCHAIN)as
AR=$(TOOLCHAIN)ar
LD=$(CC) -nostdlib $(LDFLAGS) -L../bios
STRIP=$(TOOLCHAIN)strip
MAKE=make

INCDIR  =
ASFLAGS =-mcpu=68000 $(INCDIR)
CFLAGS  =-fomit-frame-pointer -fno-common -mshort -O3 $(INCDIR)

OBJ = $(SRC:.c=.o)


#../emutospp.ld: ../emutos.ld ../include/config.h ../tosvars.ld

%.ld:%.ld_
	$(CPP) $(CPPFLAGS) -P -x c $< -o $@

.PHONY:clean

clean:
	rm -rf *.o *.a
