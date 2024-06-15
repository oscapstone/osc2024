

SECTIONS
{
	. = 0xffff000000000000;
	. += 0x80000;
	_code_start = . ;
	.text.boot : { *(.text.boot) }
	.text : { *(.text) }
	_code_end = .;
	.rodata : { *(.rodata) }
	.data : { *(.data) }
	. = ALIGN(0x8);
	bss_begin = .;
	.bss : { *(.bss*) } 
	bss_end = .;
}
