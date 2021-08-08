TARGET = libutil.a
.DEFAULT_GOAL = $(TARGET)

include ../Makefile.inc

INCDIR  +=-I../include
SRC := cookie.o doprintf.o memmove.o memset.o setjmp.o string.o stringasm.o

$(TARGET): $(OBJ) ../linker/memorymap.ld Makefile
	$(AR) rcs -o $@ $(OBJ)

