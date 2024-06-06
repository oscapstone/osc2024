
__rpi_phys_dram_start_addr = 0;

__rpi_phys_binary_load_addr = 0x80000;

ENTRY(__rpi_phys_binary_load_addr)


SECTIONS
{
    . = __rpi_phys_binary_load_addr;
    .text :
    {
        KEEP(*(.text._start))
        *(.text._start_arguments) /* Constants (or statics in Rust speak) read by _start(). */
        *(.text._start_rust)      /* The Rust entry point */
        *(.text*)                 /* Everything else */
    } :segment_code

    .rodata : ALIGN(8) { *(.rodata*) } :segment_code

    .data : { *(.data*) }

    .bss (NOLOAD): ALIGN(16)
    {
        __bss_start = .;
        *(.bss*);
        . = ALIGN(16);
        __bss_end_exclusive = .;
    }
    . = ALIGN(8);
    .usr_prog(NOLOAD): {
        __usr_prog_start = .;
        . = . + 0x1000;
        __usr_prog_end = .;
    }
    __dtb_addr = .;
    
}   
