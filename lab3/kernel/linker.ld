SECTIONS
{
    /* bootloader load kernel to 0x80000 */
    . = 0x80000;
    /* code section */
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
    
   /DISCARD/ : { *(.comment) *(.gnu*) *(.note*) *(.eh_frame*) }
}

/* 8 bytes(64bits) for register size to move */
PROVIDE(__bss_size__ = (__bss_end__ - __bss_start__) >> 3);
