_heap_stack_size = 1M;
_bootloader_size = 64K;

SECTIONS
{
    . = 0x80000;
    _start = .;
    .text : { *(.text.boot) *(.text) }
    .rodata : { *(.rodata) }
    .data : { *(.data) }
    .bss : {
        _bss_top = .;
        *(.bss) 
    }
    _bss_size = SIZEOF(.bss) >> 3;
    .heap : {
        . = ALIGN(8);
        _heap_top = .;
    } 
    . = . + _heap_stack_size;
    .stack : {
        . = ALIGN(8);
        _stack_top = .;
    }
    _end = .;

    . = 0x3000000;
    _bootloader_relocated_addr = 0x3000000;
    . = . + _bootloader_size;
}

__code_size = (_end - _start);