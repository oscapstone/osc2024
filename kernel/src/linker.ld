/* The physical address at which the the kernel binary will be loaded by the Raspberry's firmware */
__rpi_phys_binary_load_addr = 0x80000;
_bss_start      = 0x20000000;
_bss_start      = 0x20001000;


ENTRY(__rpi_phys_binary_load_addr)

/* Flags:
 *     4 == R
 *     5 == RX
 *     6 == RW
 */
PHDRS
{
    segment_code PT_LOAD FLAGS(5);
}

SECTIONS
{
    . =  __rpi_phys_binary_load_addr;

    /***********************************************************************************************
    * Code
    ***********************************************************************************************/
    .text : {
        KEEP(*(.text._start))
    } :segment_code

    .bss : {
        _bss_start = .;
        *(.bss)
        _bss_end = .;
    }
}
