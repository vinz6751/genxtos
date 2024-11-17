


.bss : SUBALIGN(2)
{
    __bss = .;
    *(.bss COMMON)
    __ebss = .;
} >REGION_READ_WRITE
