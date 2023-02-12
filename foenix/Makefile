DEBUG=yes
#LIBCMINI=/home/vincent/Atari/Crossdev/libcmini
LIBCMINI=/home/vincent/Atari/Repos/libcmini

# Pick a compiler gcc or calypsi
COMPILER=calypsi

ifeq ($(COMPILER),gcc)
  TOOLCHAIN=m68k-atari-mint-
  CC=$(TOOLCHAIN)gcc
  AS=$(TOOLCHAIN)as
  AR=$(TOOLCHAIN)ar
  STRIP=$(TOOLCHAIN)strip
  CFLAGS=-I$(LIBCMINI)/include -DLIBCMINI -fomit-frame-pointer -Os -std=c99
  LDFLAGS=-L$(LIBCMINI)/build -lcmini -lgcc
  ARFLAGS=-crs
else ifeq ($(COMPILER),calypsi)
  CC=cc68k
  AS=as68k
  AR=nlib
  ARFLAGS=
else
  $(error Compiler not recognized)
endif


LIB=liba2560u.a

SRC_C=a2560u.c fnxlog.c drivers/bq4802ly.c drivers/foenix_cpu.c drivers/gavin_irq.c drivers/gavin_timer.c \
	ps2_keyboard.c ps2_mouse_a2560u.c ps2.c \
	drivers/sn76489.c drivers/ym2612.c drivers/uart16550.c drivers/vicky2.c drivers/wm8776.c \
	shadow_fb.c
SRC_S=

ifeq ($(COMPILER),gcc)
  SRC_S += a2560u_s.S shadow_fb_s.S
else ifeq ($(COMPILER),calypsi)
  SRC_C += a2560u_interrupts.c
endif

SRC=$(SRC_C) $(SRC_S)
OBJ_C=$(SRC_C:.c=.o)
OBJ_S=$(SRC_S:.S=.o)
OBJ=$(OBJ_C) $(OBJ_S)

# all: $(LIB)
# ifeq ($(DEBUG),yes)
#     @echo "Génération en mode debug"
# else
#     @echo "Génération en mode release"
# endif

$(LIB): $(OBJ)
	$(AR) $(ARFLAGS) $@ $^

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

%.o: %.S
	$(CC) -o $@ -c $< $(CFLAGS)

#%.o: %.S
#	$(AS) -o $@ $< $(ASFLAGS)

.PHONY: clean

clean:
	@rm -rf *.o drivers/*.o *.a *~

