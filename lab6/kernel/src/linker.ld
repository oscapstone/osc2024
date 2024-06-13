_heap_stack_size = 50M;

SECTIONS
{
    . = 0xffff000000000000;
    . += 0x80000;
    _kernel_start = .;

    .text : {
        *(.text.boot)
        *(.text)
    }

    .rodata : {
        *(.rodata)
    }

    .data : {
        *(.data)
    }

    .bss : {
        _bss_top = .;
        *(.bss)
        *(COMMON)
    }
    _bss_end = .;
    _bss_size = _bss_end - _bss_top;

    .heap : {
        . = ALIGN(8);
        _heap_top = .;
        . += _heap_stack_size;
        _heap_end = .;
    }

    _kernel_end = .;

    . = 0xffff00002c000000;
    _kernel_stack_end = .;
    . = 0xffff00003c000000;
    _kernel_stack_top = .;
}