


.text : SUBALIGN(2)
{
    CREATE_OBJECT_SYMBOLS
    __text = .;
    *(.text)
    *(.rodata .rodata.*)
    __etext = .;
} >REGION_READ_ONLY =0x4afc
