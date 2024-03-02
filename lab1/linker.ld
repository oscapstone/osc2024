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
	.bss ALIGN(16) (NOLOAD):
	{
		bss_start = .;
		*(.bss)
		bss_end = .;
	}
}
