MEMORY {
  MEM_TEXT  (rx)  : ORIGIN = 0x080000, LENGTH = 1M
  MEM_DATA  (rwx) : ORIGIN = 0x180000, LENGTH = 1M
  MEM_STACK (rwx) : ORIGIN = 0x280000, LENGTH = 1M
}

SECTIONS
{
  .data : ALIGN(8) {
    *(.data)
  } > MEM_DATA
  .bss : ALIGN(8) {
    *(.bss)
  } > MEM_DATA
  .text : ALIGN(8) {
    *(.text)
  } > MEM_TEXT
}
