/* The physical address at which the uartload binary will be loaded by the Raspberry's firmware */
__rpi_phys_binary_load_addr = 0x80000;

/* The physical address at which the uartload binary is linked to and will be relocated to */
__uarload_phys_binary_link_addr = 0x60000;

ENTRY(__rpi_phys_binary_load_addr);

PHDRS
{
    segment_boot_core_stack PT_LOAD FLAGS(/* RW */ 6);
    segment_code            PT_LOAD FLAGS(/* RX */ 5);
    segment_data            PT_LOAD FLAGS(/* RW */ 6);
}

SECTIONS
{
    . = __uarload_phys_binary_link_addr;

    .boot_core_stack (NOLOAD) :
    {
                                                /*   ^             */
                                                /*   | stack       */
        . += __rpi_phys_binary_load_addr;       /*   | growth      */
                                                /*   | direction   */
        __boot_core_stack_end_exclusive = .;    /*   |             */
    } :segment_boot_core_stack

    __binary_nonzero_start = .;
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

    /* Fill up to 8 byte, because relocating the binary is done in 8 byte chunks */
    /* We do not need to relocate BSS because we zeroed it directly at the relocated address in [crates/uartload/src/boot.s]:28 */
    . = ALIGN(8);
    __binary_nonzero_end_exclusive = .;

    /* BSS section is zeroed in pair of 8 byte chunks. Align start and end to 16 bytes. */
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
