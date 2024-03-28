_bootloader = 0x60000;
_kernel = 0x80000;

SECTIONS
{
	. = _bootloader;
	__start = .;
	.text.boot : { *(.text.boot) }
	.text : { *(.text) }
	.rodata : { *(.rodata) }
	.data : { *(.data) }
	. = ALIGN(0x8);
	bss_begin = .;
	.bss : { *(.bss*) } 
	bss_end = .;
	__end = .;
}

__loader_size = (__end - __start) >> 3;
