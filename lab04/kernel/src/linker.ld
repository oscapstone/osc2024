SECTIONS
{
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
      . = ALIGN(0x8);
      __bss_start = .;
	    *(.bss)
      . = ALIGN(0x8);
	    __bss_end = .;
  }
  . = ALIGN(0x8);
  _kernel_end = .;
  . = ALIGN(0x8);
  _heap_start = .;
}
__bss_size = (__bss_end - __bss_start)>>3;