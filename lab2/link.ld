SECTIONS
{
    . = 0x80000;
    /* set the memory address*/
    .text : { KEEP(*(.text.boot)) *(.text .text.* .gnu.linkonce.t*) }
    .rodata : { *(.rodata .rodata.* .gnu.linkonce.r*) }
    /* data section starts here dont' override existing data symbol if present*/
    PROVIDE(_data = .);
    .data : { *(.data .data.* .gnu.linkonce.d*) }
    .bss (NOLOAD) : {
        /*tells the linker not to allocate space for this section output file, zeroed at runtime*/
        . = ALIGN(16);
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    }
    _end = .;

   /DISCARD/ : { *(.comment) *(.gnu*) *(.note*) *(.eh_frame*) }
   /*discard unnecessary sections*/
}
__bss_size = (__bss_end - __bss_start)>>3;
