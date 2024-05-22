/* The physical address at which the the kernel binary will be loaded by the Raspberry's firmware */
__rpi_phys_binary_load_addr = 0x20010000;

ENTRY(__rpi_phys_binary_load_addr)

SECTIONS
{
    . =  __rpi_phys_binary_load_addr;

    .text : ALIGN(8) {
        KEEP(*(.text.main))
        *(.text*)
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