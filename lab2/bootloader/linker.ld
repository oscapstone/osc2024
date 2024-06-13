__stack_top = 0x80000;

SECTIONS
{
	. = 0x80000;
	__bootloader_start = .;
	__ini_pos = .;

	.text.boot : { *(.text.boot) }
	.text : { *(.text) }
	.rodata : { *(.rodata) }

	. = ALIGN(0x8);
	
	bss_begin = .;
	.bss : { *(.bss*) }
	bss_end = .;

	__bootloader_end = .;
}

__new_pos = 0x60000;
