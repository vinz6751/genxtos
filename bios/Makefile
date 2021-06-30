TARGET = libbios.a
.DEFAULT_GOAL = $(TARGET)

include ../Makefile.inc

INCDIR +=-I../include
CFLAGS += -DWITH_CLI=1
SRC := aciaVecs.S acsi.c amiga.c amiga2.S bios.c biosmem.c blkdev.c bootparams.c chardev.c clock.c conout.c country.o delay.o delayasm.S disk.o dma.o dmasound.o dsp.o dsp2.o endrom.o floppy.o fnt_gr_6x6.o fnt_gr_8x16.o fnt_gr_8x8.o fnt_l2_6x6.o fnt_l2_8x16.o fnt_l2_8x8.o fnt_off_6x6.o fnt_off_8x8.o fnt_ru_6x6.o fnt_ru_8x16.o fnt_ru_8x8.o fnt_st_6x6.o fnt_st_8x16.o fnt_st_8x8.o fnt_tr_6x6.o fnt_tr_8x16.o fnt_tr_8x8.o font.o ide.o ikbd.o initinfo.o kprint.o kprintasm.S linea.S lineainit.o lineavars.S lisa.o lisa2.S lowstram.o machine.o memory.S memory2.o mfp.o midi.o mouse.o natfeat.S nova.o nvram.o panicasm.S parport.o pmmu030.o processor.S screen.o scsi.o sd.o serport.o sound.o spi_vamp.o startup.S vectors.S videl.o vt52.o xbios.o xhdi.o 


$(TARGET): $(OBJ) bios.ld Makefile
	$(AR) rcs -o $@ $(OBJ)

bios.ld: ../linker/memorymap.ld
