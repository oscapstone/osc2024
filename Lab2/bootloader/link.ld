SECTIONS
{
    . = 0x60000;
    _loader_start = .;
    .text : { KEEP(*(.text.boot)) *(.text .text.*) }
    .rodata : { *(.rodata .rodata.*) }
    .data : { *(.data .data.*) }
    .bss (NOLOAD) : {
        . = ALIGN(16);
        _bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        _bss_end = .;
    }
    _loader_end = .;
}
_bss_size = (_bss_end - _bss_start) >> 3;
_loader_size = (_loader_end - _loader_start) >> 3;
