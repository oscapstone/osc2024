ENTRY(_start)

SECTIONS
{
    . = 0x60000;
    __bootloader_start = .;
    __text_start = .;
    .text : {
        *(.text.relo)
        _bl_entry = .;
        KEEP(*(.text.boot))
        *(.text .text.* .gnu.linkonce.t*)
        *(.rodata .rodata.* .gnu.linkonce.r*)
    }
    . = ALIGN(16);
    __text_end = .;

    PROVIDE(_data = .);
    .data : { 
        *(.data .data.* .gnu.linkonce.d*)
        . = ALIGN(16); 
        }

    .bss (NOLOAD) : {
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(16);
        __bss_end = .;
    }
    __bootloader_end = .;
    

   /* /DISCARD/ : { *(.comment) *(.gnu*) *(.note*) *(.eh_frame*) } */
}
__bss_size = (__bss_end - __bss_start) >> 3;
__bootloader_size = (__bootloader_end - __bootloader_start)>>3;