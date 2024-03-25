_bootloader = 0x60000;
_kernel = 0x80000;
_cpio_file = 0x20000000;


SECTIONS
{
	. = _bootloader;
	.text.boot : { *(.text.boot) }
	.text : { *(.text) }
	.rodata : { *(.rodata) }
	.data : { *(.data) }
	. = ALIGN(0x8);
	bss_begin = .;
	.bss : { *(.bss*) } 
	bss_end = .;
}
