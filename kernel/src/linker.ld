__rpi_phys_binary_load_addr = 0xffff000000080000;

ENTRY(__rpi_phys_binary_load_addr)

SECTIONS
{
    . = __rpi_phys_binary_load_addr;

    .text : ALIGN(8) {
        KEEP(*(.text._start))
        *(.text*)
        /* *(.text.exception_vector_table) */
    } 
    
    .rodata : ALIGN(8) { *(.rodata*) } 

    .data : ALIGN(8) { *(.data*) }

    .bss (NOLOAD) : ALIGN(16) {
        _bss_start = .;
        *(.bss*)
        . = ALIGN(16);
        _bss_end = .;
    }

}
