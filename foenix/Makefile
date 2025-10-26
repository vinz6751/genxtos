# I struggled quite a bit making something to work. It's probably not optimal.

# TODO: select correct CPU !
# I have problems with muls/divs operations if not selecting a 68020+ target.
# Normally this is passed from the EmuTOS makefile but if we want to use the lib as
# standalone (without EmUTOS) we'll need to rework it. In particular, the same machine
# (GenX, A2560M) can have different processors or flavors of it...
CPUFLAGS=-m68060


LIBCMINI=/home/vincent/Atari/Repos/libcmini
TOOLCHAIN=m68k-atari-mint-
CC=$(TOOLCHAIN)gcc
AS=$(TOOLCHAIN)as
AR=$(TOOLCHAIN)ar
RANLIB=$(TOOLCHAIN)ranlib
STRIP=$(TOOLCHAIN)strip
#-mshort is really only for 68k targets, others have a 32bit data bus so there's no performance to gain from that
# EmuTOS uses that so we're doing it too.
CFLAGS=-mshort -fomit-frame-pointer -Os -std=c99 -D$(MACHINE_NAME) $(CPUFLAGS)
#CFLAGS+=-I$(LIBCMINI)/include -DLIBCMINI
#LDFLAGS=-L$(LIBCMINI)/build -lcmini -lgcc

SRC_C=a2560u.c a2560u_debug.c bq4802ly.c cpu.c interrupts.c mpu401.c \
	ps2_keyboard.c ps2_mouse_a2560u.c ps2.c \
	sn76489.c superio.c timer.c ym2612.c uart16550.c vicky2.c vicky_mouse.c wm8776.c \
	shadow_fb.c ym262.c
SRC_S=a2560u_s.S shadow_fb_s.S timer_s.S
SRC=$(SRC_C) $(SRC_S)
OBJ_C=$(SRC_C:.c=.o)
OBJ_S=$(SRC_S:.S=.o)
OBJ=$(OBJ_C) $(OBJ_S)

.PHONY: default
default:
	@echo "Select a target libfoenix-<machine>.a"

libfoenix-a2560k.a: MACHINE_NAME=MACHINE_A2560K
libfoenix-a2560m.a: MACHINE_NAME=MACHINE_A2560M
libfoenix-a2560x.a: MACHINE_NAME=MACHINE_A2560X
libfoenix-a2560u.a: MACHINE_NAME=MACHINE_A2560U
libfoenix-genx.a: MACHINE_NAME=MACHINE_GENX

libfoenix-%.a: $(OBJ)
	$(AR) -crs $@ $^
	$(RANLIB) $@

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

%.o: %.S
	$(CC) -o $@ -c $< $(CFLAGS)


.PHONY: clean mrproper

clean:
	@rm -rf *.o *.a *~

mrproper: clean
	@rm -rf $(EXEC)
