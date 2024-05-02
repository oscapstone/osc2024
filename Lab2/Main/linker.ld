SECTIONS
{
  . = 0x80000;
  
  .text.kernel : { *(.text.kernel) }

  .text :  { *(.text) }

  .rodata : {  *(.rodata) }
  . = ALIGN(0x1000); 

  .data : { *(.data) }
  . = ALIGN(0x1000); 

  bss_begin = .; 
  .bss (NOLOAD) : { *(.bss) }
  . = ALIGN(0x1000); 
  bss_end = .;  
}