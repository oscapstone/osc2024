SECTIONS
{
    /* bootloader load kernel to 0x80000 */
    . = 0x80000;
    /* code section */
    .text :
    {
        /* keep the boot to the first section of text segment */
        KEEP(*(.text.boot))
        *(.text .text.*)
    }
    /* read only data section */
    .rodata :
    {
        *(.rodata)
    }
    /* data section */
    .data :
    {
        . = ALIGN(16);
        *(.data)
    }
    /* bss section is to store the global variable */
    .bss :
    {
        . = ALIGN(16);
        PROVIDE(__bss_start__ = .);
        *(.bss)
        PROVIDE(__bss_end__ = .);
    }
}

/* 8 bytes(64bits) for register size to move */
PROVIDE(__bss_size__ = (__bss_end__ - __bss_start__) >> 3);
