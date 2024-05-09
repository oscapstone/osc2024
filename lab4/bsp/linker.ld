
SECTIONS
{
  . = 0x60000;
  __loader_start = .;
  . = 0x80000;
  .text : 
  {
      KEEP(*(.text.boot))
      *(.text) 
  } 
  .rodata :{ *(.rodata) }
  .data : { *(.data) }
  .bss :
  {
      __bss_start = .;
	    *(.bss)
	    *(COMMON)
	    __bss_end = .;
  }
  . = ALIGN(8);
  __heap_start = .;
  __loader_end = .;
}
__bss_size = (__bss_end - __bss_start) >> 3;
__loader_size = (__loader_end - __loader_start) >> 3;
