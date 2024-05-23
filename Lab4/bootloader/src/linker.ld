SECTIONS
{
    . = 0x80000 - 0x20000;
    PROVIDE(_code = .);
    .text.boot : { KEEP(*(.text.boot)) }
    .text : { *(.text .text.* .gnu.linkonce.t*) }
    .rodata : { *(.rodata .rodata.* .gnu.linkonce.r*) }
    .data : { *(.data .data.* .gnu.linkonce.d*) }
    .bss : {
        . = ALIGN(0x10);
        _bss_start = .;
        *(.bss .bss.*)
        _bss_end = .;
    }
    _end = .;

    /DISCARD/ : { *(.comment) *(.gnu*) *(.note*) *(.eh_frame*) }
}

_bl_size = (_end - _code) >> 3;
_bss_size = (_bss_end - _bss_start) >> 3;
