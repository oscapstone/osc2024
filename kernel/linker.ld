/*
TODO: set stack pointer
*/

MEMORY {
  TEXT      (rx)  : ORIGIN = 0x080000, LENGTH = 256K
  RODATA    (rx)  : ORIGIN = 0x090000, LENGTH = 64K
  DATA      (rwx) : ORIGIN = 0x098000, LENGTH = 64K
  USERSPACE (rwx) : ORIGIN = 0x500000, LENGTH = 1M
}

SECTIONS
{
  .data : ALIGN(8) {
    *(.data)
  } > DATA

  .rodata : ALIGN(8) {
    *(.rodata)
  } > RODATA

  .bss : ALIGN(8) {
    __bss_start = .;
    *(.bss)
    __bss_end = .;
  } > DATA

  .text : ALIGN(8) {
    *(.text .text.boot)
  } > TEXT

  /* 128K stack */
  __stack_top = 0x0A0000;
}

__bss_size = (__bss_end - __bss_start);
__userspace_start = ORIGIN(USERSPACE);
__userspace_end = ORIGIN(USERSPACE) + LENGTH(USERSPACE);
