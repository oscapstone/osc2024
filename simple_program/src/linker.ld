/* The physical address at which the the kernel binary will be loaded by the Raspberry's firmware */
__rpi_phys_binary_load_addr = 0x20010000;


ENTRY(__rpi_phys_binary_load_addr)

/* Flags:
 *     4 == R
 *     5 == RX
 *     6 == RW
 */
PHDRS
{
    segment_code PT_LOAD FLAGS(5);
    segment_data PT_LOAD FLAGS(6);
}

SECTIONS
{
    . =  __rpi_phys_binary_load_addr;

    /***********************************************************************************************
    * Code
    ***********************************************************************************************/
    .text : {
        KEEP(*(.text.main))
        *(.text*)
        /* *(.text.exception_vector_table) */
    } :segment_code

    .rodata : ALIGN(8) { *(.rodata*) } :segment_code

    .data : { *(.data*) } :segment_data

    .bss (NOLOAD) : ALIGN(16) {
        _bss_start = .;
        *(.bss*)
        . = ALIGN(16);
        _bss_end = .;
    } :segment_data

    .symtab : {
        __start_symtab = .;
        *(.symtab)
        __stop_symtab = .;
    } :segment_data

    .shstrtab : {
        __start_shstrtab = .;
        *(.shstrtab)
        __stop_shstrtab = .;
    } :segment_data

    .strtab : {
        __start_strtab = .;
        *(.strtab)
        __stop_strtab = .;
    } :segment_data
}