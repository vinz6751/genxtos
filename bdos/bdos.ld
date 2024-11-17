INCLUDE linker/memorymap.ld
OUTPUT_FORMAT(binary)
SECTIONS
{
 .first_stram :
 {
_etv_term = 0x408;
_bufl = 0x4b2;
     . = 0x800;
     sysvars_end = .;
 } >stram
    INCLUDE ../linker/section_text.ld
    INCLUDE ../linker/section_bss.ld
}
