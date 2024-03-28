
SECTIONS
{
  . = 0x60000;
  __bootloader_start = .;
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
  __bootloader_end = .;
}
__bss_size = (bss_end - bss_begin) >> 3;
__bootloader_size = (__bootloader_end - __bootloader_start) >> 3;