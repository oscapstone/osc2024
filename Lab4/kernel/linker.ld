# Define 16MB for heap.
HEAP_SIZE = 0x1000000;

STACK_PTR = 0x1295B88;

SECTIONS
{
  . = 0x80000;
  .text.boot : { *(.text.boot) }
  .text : { *(.text) }
  .rodata : { *(.rodata) }
  .data : { *(.data) }
  # Use NOLOAD in .bss ensures that this section is allocated space in the memory layout without
  # attempting to load any pre-initialized data from the binary.
  .bss (NOLOAD) : {
    . = ALIGN(0x8);
    bss_begin = .;
    *(.bss*)
    bss_end = .;
  }

  . = ALIGN(0x8);
  __heap_start = .;
  . = . + HEAP_SIZE;
  __heap_end = .;
}