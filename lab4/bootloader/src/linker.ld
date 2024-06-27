SECTIONS
{
	. = 0x80000 - 0x20000;
	PROVIDE(_code = .);
	.text.boot : { *(.text.boot) }
	.text : { *(.text) }
	.rodata : { *(.rodata) }
	.data : { *(.data) }
	. = ALIGN(0x8);
	bss_begin = .;
	.bss : { *(.bss*) } 
	bss_end = .;
	_end = .;
}
_bss_size = (bss_end - bss_begin) >> 3;
_load_size = (_end - _code) >> 3;