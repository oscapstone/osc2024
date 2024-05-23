init_addr = 0x80000;
_heap_stack_size = 0x10000;
_bootloader_relocated_addr = 0x60000;

SECTIONS
{
	. = init_addr;
	.text.boot : { *(.text.boot) }
	.text : { *(.text) }
	.rodata : { *(.rodata) }
	.data : { *(.data) }
	. = ALIGN(0x8);
	bss_begin = .;
	.bss : { *(.bss*) } 
	bss_end = .;
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
}