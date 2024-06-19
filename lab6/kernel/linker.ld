SECTIONS
{
    . = 0xffff000000000000;
    . += 0x80000;
    /* gnu linkonce : the linker will only keep one copy and discard the
     other. */
    .text : { KEEP(*(.text.boot)) *(.text .text.* .gnu.linkonce.t*) }
    /* read only data */
    .rodata : { *(.rodata .rodata.* .gnu.linkonce.r*) }
    /* avoid redefinition */
    PROVIDE(_data = .);
    .data : { *(.data .data.* .gnu.linkonce.d*) }
    .bss (NOLOAD) : {
        . = ALIGN(16);
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    }
    _heap_start = .;
    _end = .;
    /* input section不會被輸出成output section */
   /DISCARD/ : { *(.comment) *(.gnu*) *(.note*) *(.eh_frame*) }
}
__bss_size = (__bss_end - __bss_start)>>3;