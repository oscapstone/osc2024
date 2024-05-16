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
  . = __phys_dram_start_addr;
  .stack (NOLOAD) : {
    . += __phys_binary_load_addr;
  } : segment_stack
  . = __phys_binary_load_addr;
  .text : {
    KEEP(*(.text._start))
    *(.text._start_arguments)
    *(.text._start_rust)
    *(.text*)
  } : segment_code
  .rodata : ALIGN(8) { *(.rodata*) } : segment_code
  .data : { *(.data*) } : segment_data
  .bss (NOLOAD) : ALIGN(16) {
    __bss_begin = .;
    *(.bss*)
    . = ALIGN(16);
    __bss_end = .;
  } : segment_data
  .heap (NOLOAD) : ALIGN(PAGE_SIZE) {
    __heap_begin = .;
    . += 16 * 1024 * 1024;
  } : segment_heap
  __heap_end = .;
}
