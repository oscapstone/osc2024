__rpi_phys_dram_start_addr = 0;
__rpi_bootloader_addr = 0x60000;
__rpi_phys_binary_load_addr = 0x80000;

ENTRY(__rpi_phys_binary_load_addr)


SECTIONS
{
    . = __rpi_bootloader_addr
    .text :
    {
        KEEP(*(.text._bootloader))
        *(.text._bootloader_arguments) /* Constants (or statics in Rust speak) read by _start(). */
        *(.text._start_rust)      /* The Rust entry point */
        *(.text*)                 /* Everything else */
    } :segment_code
    
    . = __rpi_phys_binary_load_addr;
    .text :
    {
        KEEP(*(.text._start))
    }

    .rodata : ALIGN(8) { *(.rodata*) } :segment_code

    .data : { *(.data*) }

    /* Section is zeroed in pairs of u64. Align start and end to 16 bytes */

    .bss (NOLOAD): ALIGN(16)
    {
        __bss_start = .;
        *(.bss*);
        . = ALIGN(16);
        __bss_end_exclusive = .;
    }

}
