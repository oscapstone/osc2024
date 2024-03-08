__rpi_phys_binary_load_addr = 0x80000;

ENTRY(__rpi_phys_binary_load_addr)

SECTIONS {
    . = __rpi_phys_binary_load_addr;
    PROVIDE(__boot_core_stack_end = .);

    .text : {
        KEEP(*(.text._start))
        *(.text .text.*)
    }

    .rodata : {
        *(.rodata .rodata.*)
    }
    .data : {
        *(.data .data.*)
    }

    .bss : {
        __bss_start = .;
        *(.bss .bss.*)
        __bss_end = .;
    }

    .shstrtab : {
        *(.shstrtab)
    }

    /DISCARD/ : {
        *(.comment)
        *(.symtab)
        *(.strtab)
    }
}
