
__rpi_phys_dram_start_addr = 0;

__rpi_phys_binary_load_addr = 0x60000;


ENTRY(__rpi_phys_binary_load_addr)


SECTIONS
{
    . = __rpi_phys_binary_load_addr;
    __relo_start = .;
    .text :
    {
        KEEP(*(.text._start))
        *(.text._start_arguments) /* Constants (or statics in Rust speak) read by _start(). */
        *(.text._start_rust)      /* The Rust entry point */
        *(.text*)                 /* Everything else */
    } :segment_code
    
    .rodata : ALIGN(16) { *(.rodata*) } :segment_code
    .data : ALIGN(16) { *(.data*) }
    . = ALIGN(16);
    __relo_end = .;


    .bss (NOLOAD): ALIGN(16)
    {
        __bss_start = .;
        *(.bss*);
        . = ALIGN(16);
        __bss_end_exclusive = .;
    }
    __dtb_addr = .;

}