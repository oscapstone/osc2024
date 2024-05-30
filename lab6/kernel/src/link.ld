PHDRS 
{
	text PT_LOAD FLAGS(5);
  	data PT_LOAD FLAGS(6);
  	bss PT_LOAD FLAGS(6);
}

SECTIONS
{
	. = 0xFFFF000000000000;
  	. += 0x80000;
	.text :
	{
		KEEP(*(.text.boot))
		*(.text .text.*)
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
	. = ALIGN(0x00001000);
	pg_dir = .;
	.data.pgd : { . += (1028 * (1 << 12)); } /*PGD: 1 page, PUD: 1 page, PMD: 2 pages, PTE: 1024 pages*/
}
__bss_size = (__bss_end - __bss_start) >> 3;

