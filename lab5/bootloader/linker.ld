SECTIONS
{
    /* bootloader load the SD card loader to 0x80000 */
    . = 0x80000;
    PROVIDE(__bootloader_start__ = .);
    /* code section */
    .text :
    {
        /* keep the boot to the first section of text segment */
        KEEP(*(.text.boot))
        *(.text)
    }
    /* read only data section */
    .rodata :
    {
        *(.rodata)
    }
    /* data section */
    .data :
    {
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
    PROVIDE(__bootloader_end__ = .);
}

/* 8 bytes(64bits) for register size to move */
PROVIDE(__bss_size__ = (__bss_end__ - __bss_start__) >> 3);
PROVIDE(__bootloader_size__ = (__bootloader_end__ - __bootloader_start__) >> 3);
