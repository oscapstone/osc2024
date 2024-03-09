SECTIONS
{
	. = 0x80000;
	.text :
	{
		KEEP(*(.text.boot))
		*(.text)
	}
	.rodata :
	{
		*(.rodata)
	}
	.data :
	{
		*(.data)
	}
	.bss : ALIGN(8)
	{
		__bss_start = .;
		*(.bss)
		__bss_end = .;
	}
}
__bss_size = (__bss_end - __bss_start) >> 3;

