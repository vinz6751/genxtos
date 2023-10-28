# Make sure the OS address is 0x100000 in emutos.ld
# and poke 0x100000 to the 68k's reset vector at 0x4
fnxmgr --binary emutos-a2560u.rom --address 0x100000
