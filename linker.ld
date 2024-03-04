__rpi_phys_binary_load_addr = 0x80000;

SECTIONS
{
    . = __rpi_phys_binary_load_addr;
    __boot_core_stack_end = .;
    .text :  {
        KEEP(*(.text._start))
        *(.text)
    }

    .rodata : { *(.rodata) }
    .data : { *(.data) }

    .bss : {
        __bss_start = .;
        *(.bss*)
        __bss_end = .;
    }
}
