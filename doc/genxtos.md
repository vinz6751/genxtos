This are design notes about the work that has been done to make EmuTOS work on the Foenix.

## Build
A new target a2560u has been created in the makefile, so to build you can do something like:
make a2560u
Other compile options (e.g. UNIQUE=fr for French) should work.
I recommend you use the -jn (n being your number of CPU cores) to speed up the compilation.

This builds emutos-a2560u.rom which is the OS image.
The OS image is built to start at a certain location, defined in emutos.ld, the GNU ld linker file. Checkout the "defined(MACHINE_A2560U)" condition there.

### Build for development / upload to RAM
For development purposes, you probably want to build EmuTOS so it runs from RAM. In this case, you can set the ROM_ORIGIN to 0x00100000.
Then to boot it on the A2560U, e.g. from MCP you have to ensure that the 68000's reset vector points to the os with
`poke32 4 0x100000`
Then you can upload the image through the USB port with:
`fnxmgr --binary $HOME/GenX/genxtos/emutos-a2560u.rom --address 0x100000`

For the installation of fnxmgr see https://github.com/pweingar/FoenixMgr). You will also need your computer to have the USB port driver setup. For that, install the XR21B1411 driver from https://www.maxlinear.com/support/design-tools/software-drivers.

### Build for flash
If you want to build a flashable image, that ROM_ORIGIN should be 0x00e00000, ie the first flash bank of the A2560U.
You can upload it with something like
`fnxmgr --flash emutos-a2560u.rom --address 0x000000`


## Principles
The Foenix specific stuff is in the foenix/ folder. This can be build as a stand-alone library (makefile available) that you can use in other projects. This library contains "drivers" for the Foenix system.
The OS makes use of this library through the bios/a2560u_bios.c and bios/a2560u_bios_s.S files. These files provide Foenix variants or the original TOS functions, which are inserted in the os by means of #ifdef MACHINE_A2560U, much like it's done for OS ports of EmuTOS to the Amiga etc.


## Display
The original Atari ST doesn't have a text mode, everything works using a framebuffer rendered by the Shifter. On the Foenix, VICKY offers a text mode with support for a 8x8 font, which allows hardware-accelerated rendering of text.

## Shadow frame buffer
It is not possible to directly render text to the video ram on the Foenix, like it's done on the Atari ST, because scrolling of text, video inverse etc. require to read the video RAM, which the Foenix cannot do currently. As a work around, shadow framebuffer in RAM is introduced, which is copied in parts or as a whole to the VRAM during the VBL (a.k.a SOF) interrupt. This is slower but can work with graphics, and allows the use of "any" font like the 16x8 included in the OS, which is more comfortable on large resolutions.
The shadow framebuffer keeps a list of dirty cells (a cell is a system ram address pointing to a location in the shadow frame buffer, that is 8 bytes wide and is as tall as the currently used font). Whenever the console (vt52.c) updates a character in the screen, the corresponding text cell's address is added to a ringbuffer of dirty cells. Upon VBL, this buffer is examined and the corresponding portions of the system RAM are copied to the VRAM. If there are too many dirty cells, we copy the whole screen as it ends up being more efficient (I am not sure about the threshold for making the switch in strategy). Currently the copy from system RAM to VRAM are done by the processor so it's slow, but once DMA is available, it should be a lot faster. I'm just not sure if it will still be fast enough or how the "dirty area" management will work with graphics.

The original EmuTOS was refactored so the conout.c (low level text driver) can use different backing drivers. One exists for the original Atari, and 2 are introduced for the Foenix:
* a2560u_conout_text : uses VICKY's text mode. It's very fast but if you use it, it's hard to do graphics at the same time. Also, the flashing of the cursor is controller by VICKY so it behaves different (like keeping flashing while you type because there's no way to force it displayed).
* a2560u_conout_bmp : use the shadow frame buffer.

The selection is done in a2560u_bios_get_conout() based on the availability of the text mode and shadow framebuffer features.
The shadow frame buffer availability is configured by CONF_WITH_A2560U_SHADOW_FRAMEBUFFER.
The text mode support availability is configured by CONF_WITH_A2560U_TEXT_MODE.
If you have the shadow framebuffer enabled and not forcing the 8x8 font with CONF_WITH_FORCE_8x8_FONT, the bmp driver will be used.


## Line A / VDI
There is a bit mixup in the original TOS and even EmuTOS as to how to handle the low level graphics. In GenxTOS, the design decision was made that the Line-A belongs to the BIOS. Refactoring was done accordingly. Likewize, mouse rendering belonw to the Line-A. The VDI can of course override things if it wishes so.
This is because the VDI is not available yet, but Line-A variables are needed by the BIOS and text rendering things (conout, vt-52).

## Mouse
The Foenix has support for hardware mouse cursor. And if we want the mouse to work with the text mode (no frame buffer), that's what we must use. To enable that, I introduced the LINEA_MOUSE_RENDERER, which is an abstraction of something that can draw the mouse. There is:
* An implementation that is ripped from EmuTOS, ie will work with the Atari Shifter (linea_mouse_atari)
* An implementation that uses VICKY's mouse (linea_mouse_atari).

The module controlling all this is linea_mouse.c.

Likewise, the way to update mouse coordinates is different in the Foenix and the Atari ST. On the ST, there is a packet generated from the IKBD keyboard controller, and once received it calls linea_mouse_packet_received_handler which updates internal flags to let the VBL interrupt handler move the mouse at next screen refresh (using the mouse_display_driver LINEA_MOUSE_RENDERER)
On the Foenix, the mouse is moved automatically when writting PS/2 packets bytes in the appropriate GAVIN registers. So it could be really quick to just write PS/2 packets and let the mouse be moved. But in this case, the TOS (the VDI/AES in particular) monitoring mouse movement to trigger UI events (e.g. drop down a menu) would not be aware of the mouse movement. For this reason, we do both things:
* Write the PS/2 bytes to GAVIN so the mouse cursor is moved (that's the only way to move the mouse as the mouse position registers are read-only, unfortunately).
* Create a fake mouse packet (like the IKBD would do) and call the normal TOS handling routine for it.

That is all done in ps2_mouse_a2560u.c.
Obviously a mouse accelerator program will not work as it would probably hook into the TOS and tweak the packets or Line-A variables, but it's not the TOS controlling that is setting the mouse position.


## Keyboard
The Atari ST has a special keyboard with its own scancodes which are not PS/2. It has keys that don't exist on PS/2, and vice-versa. Trying to faithfully adapt one to the other is nor a straight forward thing, and some keys are "lost in translation". So to not hinder the ability of users to use their full keyboard, the design decision is to support PS/2. Therefore, the system is adapted to work with PS/2 scan codes rather than Atari keyboard ones. This includes EmuCON.
The scancodes are defined in ps2_scancodes.h and are exactly those used by Linux.

EmuTOS doesn't have PS/2 support so it was introduced as a separate subsystem which lives in the Foenix folder, and which is "glued" to the OS at startup in a2560u_bios_kbd_init(). 


## What works
[x] UART16750 sending and receiving at up to 115200bps. The reception can be done using interrupts but I failed to make the FIFO work so I'm not sure we're getting the benefit of the 64bytes FIFO. Also, the acknowledgement of interrupts does not seem to work so I use the status register.
[x] PS/2 works, but the identification of devices is flaky. For this reason it can be disabled in ps2.c. The PS/2 interrupt also has a somewhat low priority and does not have a FIFO on the A2560U, so there are chances that bytes may be dropped of a higher priority interrupt is being serviced.
[x] PS/2 keyboard
[x] PS/2 mouse
[x] VICKY text mode
[x] VICKY frame buffer-based text rendering (quirks in MicroEmacs, whose fault ?)
[x] IDE. Works well, was only tested with one device.
[x] SD. Works well, but ejection/read-only not supported or tested
[ ] WM8776 mixer code is written but not tested, no API
[ ] PSG SN76489 some code written but no API
[ ] SID
[ ] YM262 OPL3 some defines written but no API
[ ] VDI. Reviewing options, like fVDI, oVDI, and started writting Vvdi
[ ] AES. Requies the VDI first.


Other refactorings:
There were mostly done to have better layer isolation.

* BIOS #$c Bgettpa added so the GEMDOS doesn't have to read direct memory addresses (be them documented system variables).
* TPA_AREA *bmem_gettpa(void);
* BIOS $d Balloc to allocate memory before membot, or below memtop. membot/memtop are is bumped/decreased accordingly. This is used by GEMDOS (bufl_init) to reserve space for its buffers, so the TPA is completely unused.
* BIOS $e Bdrvrem returns a LONG where each bit correspond to a drive, if the bit is 1, it means the drive support media change.
