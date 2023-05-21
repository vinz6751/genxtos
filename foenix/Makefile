DEBUG=yes
#LIBCMINI=/home/vincent/Atari/Crossdev/libcmini
LIBCMINI=/home/vincent/Atari/Repos/libcmini
TOOLCHAIN=m68k-atari-mint-
CC=$(TOOLCHAIN)gcc
AS=$(TOOLCHAIN)as
AR=$(TOOLCHAIN)ar
STRIP=$(TOOLCHAIN)strip
CFLAGS=-I$(LIBCMINI)/include -DLIBCMINI -fomit-frame-pointer -Os -std=c99
LDFLAGS=-L$(LIBCMINI)/build -lcmini -lgcc

LIB=liba2560u.a

SRC_C=a2560u.c bq4802ly.c \
	ps2_keyboard.c ps2_mouse_a2560u.c ps2.c \
	sn76489.c ym2612.c uart16550.c vicky2.c vicky_mouse.c wm8776.c \
	shadow_fb.c
SRC_S=a2560u_s.S shadow_fb_s.S
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
	$(AR) -crs $@ $^

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

%.o: %.S
	$(CC) -o $@ -c $< $(CFLAGS)


.PHONY: clean mrproper

clean:
	@rm -rf *.o *~

mrproper: clean
	@rm -rf $(EXEC)
