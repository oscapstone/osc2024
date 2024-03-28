
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
      bss_begin = .;
      *(.bss)
      *(COMMON)	 
      bss_end = .;
  }
  end = .;
}
__bss_size = (bss_end - bss_begin) >> 3;