SECTIONS
{
  . = 0x20008000;
  .text : {
    KEEP(*(.text._start))
    *(.text._start_arguments)
    *(.text._start_rust)
    *(.text*)
  }
  .rodata : ALIGN(8) { *(.rodata*) }
  .data : { *(.data*) }
  .bss (NOLOAD) : ALIGN(16) {
    __bss_begin = .;
    *(.bss*)
    . = ALIGN(16);
    __bss_end = .;
  }
}