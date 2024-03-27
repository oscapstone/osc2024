/*
TODO: set stack pointer
*/

MEMORY {
  MEM_TEXT    (rx)  : ORIGIN = 0x080000, LENGTH = 1M
  MEM_RODATA  (rx)  : ORIGIN = 0x180000, LENGTH = 1M
  MEM_DATA    (rwx) : ORIGIN = 0x280000, LENGTH = 1M
  /* MEM_STACK   (rwx) : ORIGIN = 0x380000, LENGTH = 1M */
}

SECTIONS
{
  .data : ALIGN(8) {
    *(.data)
  } > MEM_DATA

  .rodata : ALIGN(8) {
    *(.rodata)
  } > MEM_RODATA

  .bss : ALIGN(8) {
    __bss_start = .;
    *(.bss)
    __bss_end = .;
  } > MEM_DATA

  .text : ALIGN(8) {
    *(.text .text.boot)
  } > MEM_TEXT

  __stack_top = 0x380000;
}

__bss_size = (__bss_end - __bss_start);
