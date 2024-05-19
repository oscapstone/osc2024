SECTIONS
{	
	. = 0xffff000000000000; 
  	. += 0x80000; 
	__kernel_start = .;
	.text : { KEEP(*(.text.boot)) *(.text .text.*) }
	.data : { *(.data) }
	. = ALIGN(0x1000);
	__pt_start = .;
	.data.pgd :
 	{
    	. += (515 * 4096); /* (PGD * 1) + (PUD * 1) + (PMD * 1) + (PTE * 512) */
  	}
	__pt_end = .;
	. = ALIGN(16);
	.bss (NOLOAD) : 
	{ 
		__bss_start = .;
		*(.bss)
		__bss_end = .;
	}
	__kernel_end = .;
}
__bss_size = (__bss_end - __bss_start)>>3;

/* 
(NOLOAD) :  means that this section should not be loaded into memory from the executable file because it's initialized to zero at runtime. 
PROVIDE() : used to avoid redefinition if .c files have defined the variable.
ALIGN() : ensures that the memory address at which a section begins is aligned to a 16-byte boundary.
*/