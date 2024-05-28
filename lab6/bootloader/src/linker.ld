init_addr = 0x80000;
_heap_stack_size = 1M;
_bootloader_relocated_addr = 0x3000000;

SECTIONS
{
    . = init_addr;
    .relocate :
    {
        KEEP(*(.text._relocate))
        KEEP(*(.text.relocate))
    }

    . = ALIGN(4096);
    _after_relocate = .;
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

    . = _bootloader_relocated_addr;
}

_bootloader_size = (_end - _after_relocate);
_bootloader_bss_top = _bootloader_relocated_addr + _bss_top - init_addr;
_bootloader_heap_top = _bootloader_relocated_addr + _heap_top - init_addr;
_bootloader_stack_top = _bootloader_relocated_addr + _stack_top - init_addr;
