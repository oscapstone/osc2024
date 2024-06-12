SECTIONS
{
    /* bootloader load kernel to 0x80000 */
    . = 0xffff000000080000;
    /*. = 0x80000;*/
    /* code section */
    PROVIDE(__kernel_start__ = .);
    .text :
    {
        /* keep the boot to the first section of text segment */
        KEEP(*(.text.boot))
        *(.text .text.*  .gnu.linkonce.t*)
    }
    /* read only data section */
    .rodata :
    {
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
        PROVIDE(__bss_start__ = .);
        *(.bss)
        PROVIDE(__bss_end__ = .);
    }
    /* used for static memory allocation */
    .data.static_mem :
    {
        PROVIDE(__static_mem_start__ = .);
        . = ALIGN(8);
        . += 32m;
        PROVIDE(__static_mem_end__ = .);
    }
    PROVIDE(__kernel_end__ = .);

    
   /DISCARD/ : { *(.comment) *(.gnu*) *(.note*) *(.eh_frame*) }
}
