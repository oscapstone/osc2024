SECTIONS
{
	. = 0x60000;
	__loader_start = .;
	.text : { *(.text.boot) *(.text) }
	.rodata : { *(.rodata) }
	.data : { *(.data) }
	.bss : {
		. = ALIGN(16);
		__bss_start = .;
		*(.bss)
		__bss_end = .;
	}
	__loader_end = .;
}
__bss_size = (__bss_end - __bss_start) >> 3;
__loader_size = (__loader_end - __loader_start) >> 3;