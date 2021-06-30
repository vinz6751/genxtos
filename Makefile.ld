# This make the linker files.
# It really consists in preprocessing them

.DEFAULT_GOAL = all

include Makefile.inc

FILES = memorymap.ld\
		section_alt_ram.ld\
		section_first_stram.ld\
		section_text.ld\
		section_bss.ld	

LINKERFILES = $(addprefix linker/,$(FILES))\
		bdos/bdos.ld\
		bdos/bdosvars.ld\
		tosvars.ld

# Where the preprocessor should search linker files
CPPFLAGS += -Ibdos -Ibios

all: $(LINKERFILES)

linker/section_first_stram.ld: bios/biosvars.ld bdos/bdosvars.ld

.PHONY:clean_ld

clean_ld:
	rm -rf $(LINKERFILES)
