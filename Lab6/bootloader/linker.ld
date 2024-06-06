SECTIONS
{
    . = 0x60000;
    boot_begin = .;
    .text.boot : { *(.text.boot) }
    .text :  { *(.text) }
    .rodata : { *(.rodata) }
    .data : { *(.data) }
    . = ALIGN(8);
    bss_begin = .;
    .bss (NOLOAD): { 
      *(.bss*) 
    }
    bss_end = .;
    boot_end = .;
}
bss_size = (bss_end - bss_begin) >> 3;
boot_size = (boot_end - boot_begin) >> 3;
