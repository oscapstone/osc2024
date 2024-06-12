SECTIONS
{
    . = 0x0;
    /* code section */
    .text :
    {
        KEEP(*(.text.boot))
        *(.text .text.*  .gnu.linkonce.t*)
    }
    /* read only data section */
    .rodata :
    {
        . = ALIGN(8);
        *(.rodata .rodata.* .gnu.linkonce.r*)
    }
    /* data section */
    .data :
    {
        . = ALIGN(8);
        *(.data .data.* .gnu.linkonce.d*)
    }
    /* bss section is to store the global variable */
    .bss :
    {
        . = ALIGN(8);
        *(.bss)
    }
    
   /DISCARD/ : { *(.comment) *(.gnu*) *(.note*) *(.eh_frame*) }
}
