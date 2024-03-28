SECTIONS
{
  . = 0x80000;
  .text : { KEEP(*(.text.boot)) *(.text .text.*) }
  .rodata : { *(.rodata .rodata.*) }
  .data : { *(.data) }
  . = ALIGN(16);
  __bss_start = .;
  .bss : { *(.bss*) }
  . = ALIGN(16);
  __bss_end = .;
  _end = .;
}


__bss_size = (__bss_end - __bss_start) >> 3;
