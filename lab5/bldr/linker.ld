SECTIONS
{
    . = 0x80000;
    __start = .;
    .text : { *(.text) }
    .bss : 
    {
        . = ALIGN(0x8);
        __bss_start = .;
        *(.bss*)
        __bss_end = .;
    }
    __end = .;
}

__bootloader_start = 0x60000;
__bss_size = (__bss_end - __bss_start) >> 3;