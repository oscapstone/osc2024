init_addr = 0x80000;
_heap_stack_size = 0x10000;
_bootloader_relocated_addr = 0x60000;
_memory_managment_space = 0x58e000; /*0x3b400 entrys*(entry size = 0x18 byte)*/
_address_to_entry_array = 0x1da000; /*0x3b400 element * sizeof(address=8byte)*/

SECTIONS
{	. = 0xffff000000000000;
	. = . + init_addr;
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
	. = . + _memory_managment_space;
	. = . + _address_to_entry_array;
	. = ALIGN(0x1000);
    .stack : {
        . = ALIGN(8);
        _stack_top = .;
    }
    _end = .;
}