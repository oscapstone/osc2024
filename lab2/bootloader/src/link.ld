SECTIONS
{
	. = 0x60000;
	__bootloader_start = .;
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
	__bootloader_end = .;
}
__bss_size = (__bss_end - __bss_start) >> 3;
__bootloader_size = (__bootloader_end - __bootloader_start) >> 3;

