PAGE_SIZE = 4K;

__phys_dram_start_addr = 0;
__phys_binary_load_addr = 0x80000;

ENTRY(__phys_binary_load_addr)

/**
 * Flags:
 * 4 == R
 * 5 == RX
 * 6 == RW
 */
PHDRS
{
  segment_stack PT_LOAD FLAGS(6);
  segment_code PT_LOAD FLAGS(5);
  segment_data PT_LOAD FLAGS(6);
  segment_heap PT_LOAD FLAGS(6);
}

SECTIONS
{
  . = 0x2000000;
  .stack (NOLOAD) : {
    . += __phys_binary_load_addr;
  } : segment_stack
  __binary_nonzero_start = .;
  .text : {
    KEEP(*(.text._start))
    *(.text._start_arguments)
    *(.text._start_rust)
    *(.text*)
  } : segment_code
  .rodata : ALIGN(8) { *(.rodata*) } : segment_code
  .data : { *(.data*) } : segment_data
  . = ALIGN(8);
  __binary_nonzero_end = .;
  .bss (NOLOAD) : ALIGN(16) {
    __bss_begin = .;
    *(.bss*)
    . = ALIGN(16);
    __bss_end = .;
  } : segment_data
  __heap_begin = .;
  .heap (NOLOAD) : ALIGN(PAGE_SIZE) {
    . += 16 * 1024 * 1024;
  } : segment_heap
  __heap_end = .;
}
