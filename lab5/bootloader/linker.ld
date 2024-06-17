SECTIONS
{
	. = 0x60000;
	__loader_start = .;
	.text : { *(.text.boot) *(.text) }
	.rodata : { *(.rodata) }
	.data : { *(.data) }
	.bss : {
		__bss_start = .;
		*(.bss)
		__bss_end = .;
	}
	__loader_end = .;
}
__bss_size = SIZEOF(.bss);
__loader_size = __loader_end - __loader_start;