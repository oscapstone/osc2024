SECTIONS
{
    . = 0x80000;
    __bootloader_start = .;
    PROVIDE(_code = .);
    .text : { KEEP(*(.text.boot)) *(.text .text.* .gnu.linkonce.t*) }
    .rodata : { *(.rodata .rodata.* .gnu.linkonce.r*) }
    PROVIDE(_data = .);
    .data : { *(.data .data.* .gnu.linkonce.d*) }
    .bss (NOLOAD) : {
        # Here I used 16 because some machines support 4-byte, 8-byte, and 16-byte. The bootloader should
        # be as fast as possible, so here I used 16. The kernel, on the other hand, uses 8-byte for maximum
        # compatibility and performance.
        . = ALIGN(16);
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    }
    . = ALIGN(8);
    __bootloader_end = .;

   /DISCARD/ : { *(.comment) *(.gnu*) *(.note*) *(.eh_frame*) }
}