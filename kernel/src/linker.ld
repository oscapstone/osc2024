/* The physical address at which the the kernel binary will be loaded by the Raspberry's firmware */
__rpi_phys_binary_load_addr = 0x80000;

ENTRY(__rpi_phys_binary_load_addr)

SECTIONS
{
    . =  __rpi_phys_binary_load_addr;

    /***********************************************************************************************
    * Code
    ***********************************************************************************************/
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
