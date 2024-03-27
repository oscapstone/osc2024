
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
   . = ALIGN(0x8);
   bss_begin = .;
  .bss :
  {
      *(.bss)
      *(COMMON)	 
  }
  bss_end = .;
  end = .;
}
__bss_size = (bss_end - bss_begin)>>3;