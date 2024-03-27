__rpi_phys_dram_start_addr = 0;

/* The physical address at which the kernel binary will be loaded by the Raspberry's firmware */
__rpi_phys_binary_load_addr = 0x80000;

ENTRY(__rpi_phys_binary_load_addr);

PHDRS
{
    segment_boot_core_stack PT_LOAD FLAGS(/* RW */ 6);
    segment_code            PT_LOAD FLAGS(/* RX */ 5);
    segment_data            PT_LOAD FLAGS(/* RW */ 6);
}

SECTIONS
{
    . = __rpi_phys_dram_start_addr;

    .boot_core_stack (NOLOAD) :
    {
                                                /*   ^             */
                                                /*   | stack       */
        . += __rpi_phys_binary_load_addr;       /*   | growth      */
                                                /*   | direction   */
        __boot_core_stack_end_exclusive = .;    /*   |             */
    } :segment_boot_core_stack

    .text :
    {
        KEEP(*(.text._start))
        *(.text._start_arguments) /* Constants (or static in Rust speak) read by _start() */
        *(.text._start_rust)      /* The Rust entry point */
        *(.text*)                 /* Everything else */
    } :segment_code

    .rodata : ALIGN(8) 
    {
        *(.rodata*)
    } :segment_code

    .data :
    {
        *(.data*)
    } :segment_data

    .bss (NOLOAD) : ALIGN(16)
    {
        __bss_start = .;
        *(.bss*);
        . = ALIGN(16);
        __bss_end_exclusive = .;
    } :segment_data

    .got :
    {
        *(.got*)
    }
    ASSERT(SIZEOF(.got) == 0, "Relocation support not expected")

    /DISCARD/ :
    {
        *(.comment*)
    }
}
