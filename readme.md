This is a fork of the EmuTOS operating system, aiming at providing an OS to the Foenix Retro Systems computers' A2560 and Genx with m68k CPU. It has some refinements in compared to the official EmuTOS, which where introduced to make it cooler on the STe, and also easier to port on other platforms.
It's build using Vincent Rivière's GCC 4.6.4 m68k-atari-mint compiler: http://vincent.riviere.free.fr/soft/m68k-atari-mint/

There is a "compile.sh" script that you can try to build.
You'll need FoenixMgr to either load to flash or load to memory (easier when developping the OS but not recommended for normal use). To switch one or the other, update emutos.ld then rebuild.
Consider this a hobby project with no guarantee that anything works :)

There is no graphics support because the TOS's VDI's implementation is very intricated to the way the Atari framebuffer works. Also, the Foenix can write to graphics memory but cannot read, as the VDI expect. We'd have to use a "shadow" framebuffer and write there (which can then be read back) as well as the "real" framebuffer, but I haven't tackled that yet.
The port was initially done for the A2560U then the A2560X (not completed)/GenX, then A2560M. You can expect to work:
- Storage (IDE/SD, at least one SD port)
- PS/2 keyboard and mouse. The keyboard mapping is in French because that's the keyboard I have. You can create your own mapping table 
- RTC
- Text output
- Serial ports (up to 115200 for machines supporting it)

EmuTOS expects storage to use a Atari-partitioned media, and only supports FAT12/FAT16. So I suggest your prepare a media with the excellent "hatari" emulator, then dump it to your flash card.

I haven't build it for the U in a while and have no U at hand at the moment so I'm not even sure it works there at present.

Any question or will to contribute, please get in touch with me. This is a hobby project for me, you can find me on the Discord challen of the Foenix community.