SECTIONS
{
	. = 0x80000;
	.text.boot : { KEEP(*(.text.boot)) }
	.text : { *(.text) }
	.data : { *(.data) }
	. = ALIGN(16);
	.bss (NOLOAD) : 
	{ 
		__bss_start = .;
		*(.bss)
		__bss_end = .;
	}
}
__bss_size = (__bss_end - __bss_start)>>3;

/* 
(NOLOAD) :  means that this section should not be loaded into memory from the executable file because it's initialized to zero at runtime. 
.gnu.linkonce.t* : sections that contain functions or code that is defined in multiple source files, but should be linked only once.
.gnu.linkonce.r* : sections that contain read-only data that is defined in multiple source files but should be linked only once.
PROVIDE() : used to avoid redefinition if .c files have defined the variable.
ALIGN() : ensures that the memory address at which a section begins is aligned to a 16-byte boundary.
*/