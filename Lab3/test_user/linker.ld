SECTIONS
{
  . = 0x500000;
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
}