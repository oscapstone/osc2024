SECTIONS
{
    . = 0xffff000000000000;
    . += 0x80000;
    _start = .;
    .text : { KEEP(*(.text.boot)) *(.text .text.* .gnu.linkonce.t*) }
    .rodata : { *(.rodata .rodata.* .gnu.linkonce.r*) }
    PROVIDE(_data = .);
    .data : { *(.data .data.* .gnu.linkonce.d*) }
    .bss (NOLOAD) : {
        . = ALIGN(16);
        _bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        _bss_end = .;
    }
    _end = .;
    _heap_start = .;
    . = 0xffff00002c000000;
    _stack_end = .;
    . = 0xffff00003c000000;
    _stack_top = .;

    /DISCARD/ : { *(.comment) *(.gnu*) *(.note*) *(.eh_frame*) }
}
_bss_size = (_bss_end - _bss_start) >> 3;