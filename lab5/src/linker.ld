
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
      bss_begin = .;
      *(.bss)
      bss_end = .;
  }
}
__bss_size = (bss_end - bss_begin)>>3;
__stack_end = 0x0200000;