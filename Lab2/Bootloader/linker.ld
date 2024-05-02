SECTIONS
{
  . = 0x60000;
  
  text_begin = .; 
  .text.relo : { *(.text.relocate) }

  boot_entry = .; 
  .text.boot : { *(.text.boot) }

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
