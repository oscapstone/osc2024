/*
TODO: set stack pointer
*/


__original_start = 0x80000;
__relocate_start = 0x50000;


SECTIONS
{
  . = __original_start;
  __prog_start = .;

  .text : ALIGN (8) {
    *(.text.start);
    __relocate_main = .;
    *(.text*);
  }

  .data : ALIGN (8) {
    *(.data*);
  }

  .rodata : ALIGN (8) {
    *(.rodata*);
  }

  .bss : ALIGN(8) {
    __bss_start = .;
    *(.bss)
    __bss_end = .;
  }

  __prog_end = .;

  __stack_top = 0x50000;
}

__bss_size = (__bss_end - __bss_start);
__prog_size = (__prog_end - __prog_start);
