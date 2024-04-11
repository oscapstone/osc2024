SECTIONS
{
    . = 0x60000;
    bootloader_begin = .;
    .text.boot : { *(.text.boot) }
    .text :  { *(.text) }
    .rodata : { *(.rodata) }
    .data : { *(.data) }
    . = ALIGN(0x8);
    bss_begin = .;
    .bss : { *(.bss*) } 
    bss_end = .;
    bootloader_end = .;
}

bootloader_size = (bootloader_end - bootloader_begin)>>3;
