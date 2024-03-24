SECTIONS
{
    . =  0x80000 - 0x20000;
    PROVIDE(_code = .);
    .text.boot : { *(.text.boot) }
    .text : { *(.text) }
    .rodata : { *(.rodata) }
    .data : { *(.data) }
    .bss : { 
        . = ALIGN(16);
        _bss_start = .;
        *(.bss) 
        _bss_end = .;
    }
    _end = .;
}

_bl_size = (_end - _code) >> 3;
_bss_size = (_bss_end - _bss_start) >> 3;
