SECTIONS
{
	. = 0x80000;
	.text : { KEEP*(.text.boot) *(.text) }
	.rodata : { *(.rodata) }
	.data : { *(.data) }
	bss_start = .;
	.bss : { *(.bss) }
	bss_end = .;
}
