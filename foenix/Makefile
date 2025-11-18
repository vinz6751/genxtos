# I struggled quite a bit making something to work. It's probably not optimal.

# TODO: select correct CPU !
# I have problems with muls/divs operations if not selecting a 68020+ target.
# Normally this is passed from the EmuTOS makefile but if we want to use the lib as
# standalone (without EmUTOS) we'll need to rework it. In particular, the same machine
# (GenX, A2560M) can have different processors or flavors of it...
CPUFLAGS=-m68060

TOOLCHAIN=m68k-atari-mint-
CC=$(TOOLCHAIN)gcc
AS=$(TOOLCHAIN)as
AR=$(TOOLCHAIN)ar
RANLIB=$(TOOLCHAIN)ranlib
STRIP=$(TOOLCHAIN)strip
# We do -std=c99 so we can use "for(int x;...)" without problems
# -mshort is really only for 68k targets, others have a 32bit data bus so there's no performance to gain from that
#  EmuTOS uses that so we're doing it too.
# -msoft-float is because my A2560M has a 68EC060 with no FPU. Without this option, the "interrupt_handler"
#  function would try to save FPU register and it'd crash. If the machine does have a FPU and it is used,
#  this option should be removed.
CFLAGS=-mshort -fomit-frame-pointer -Os -std=c99 -D$(MACHINE_NAME) $(CPUFLAGS) -msoft-float

SRC_C=a2560.c a2560_debug.c bq4802ly.c cpu.c interrupts.c mpu401.c \
	keyboard.c ps2_keyboard.c ps2_mouse_a2560.c ps2.c \
	sn76489.c superio.c timer.c uart16550.c vicky2.c vicky_mouse.c wm8776.c \
	shadow_fb.c \
	trap.c trap_dispatch.c \
	vicky2_txt_a_logger.c \
	ym2151.c ym262.c
SRC_S=a2560_s.S shadow_fb_s.S timer_s.S trap_bindings.S trap_handler.S
SRC=$(SRC_C) $(SRC_S)
OBJ_C=$(SRC_C:.c=.o)
OBJ_S=$(SRC_S:.S=.o)
OBJ=$(OBJ_C) $(OBJ_S)
DEP=$(SRC_C:.c=.d)

.PHONY: default
default:
	@echo "Select a target libfoenix-<machine>.a"

libfoenix-a2560k.a: MACHINE_NAME=MACHINE_A2560K
libfoenix-a2560m.a: MACHINE_NAME=MACHINE_A2560M
libfoenix-a2560x.a: MACHINE_NAME=MACHINE_A2560X
libfoenix-a2560u.a: MACHINE_NAME=MACHINE_A2560U
libfoenix-genx.a: MACHINE_NAME=MACHINE_GENX

libfoenix-a2560k.a libfoenix-a2560m.a u.a libfoenix-a2560x.a libfoenix-genx.a: $(OBJ)
	$(AR) -crs $@ $^
	$(RANLIB) $@

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS) -MMD -MP

%.o: %.S
	$(CC) -o $@ -c $< $(CFLAGS)


.PHONY: clean mrproper

clean:
	@rm -rf *.o *.a *.d *~

mrproper: clean
	@rm -rf $(EXEC)

# Include automatically generated dependency files
-include $(DEP)
