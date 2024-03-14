__rpi_phys_binary_load_addr = 0x80000;

SECTIONS {
    . = __rpi_phys_binary_load_addr;

    .text : {
        KEEP(*(.text._start))
        *(.text._start_rust)
        *(.text)
    }

    .rodata : ALIGN(8) {
        *(.rodata)
    }

    .data : {
        *(.data)
    }

    .bss : ALIGN(8) {
        __bss_start = .;
        *(.bss);
        . = ALIGN(8);
        __bss_end_exclusive = .;
    }
    _end = .;
}

