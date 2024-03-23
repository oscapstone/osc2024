PHDRS 
{
	text PT_LOAD FLAGS(5);
  	data PT_LOAD FLAGS(6);
  	bss PT_LOAD FLAGS(6);
}

SECTIONS
{
	. = 0x80000;
	.text :
	{
		KEEP(*(.text.boot))
		*(.text)
	}:text
	.rodata :
	{
		*(.rodata)
	}
	.data :
	{
		*(.data)
	}:data
	.bss : ALIGN(8)
	{
		__bss_start = .;
		*(.bss)
		__bss_end = .;
	}:bss
}
__bss_size = (__bss_end - __bss_start) >> 3;

