/* entry point */
ENTRY(_start)

SECTIONS
{
    /* Move boot.S itslef from 0x80000 to 0x60000 */
    . = 0x60000;

    _text_start = .;
    .text : 
    { 
        KEEP(*(.text.boot))
        _bl_start = .;
        *(.text._bl)
        *(.text) 
    }
    . = ALIGN(0x1000); /* align to page size */
    _text_end = .;

    _data_start = .;
    .data :
    {
        *(.data)
    }
    . = ALIGN(0x1000); /* align to page size */
    _data_end = .;
    
    _bss_start = .;
    .bss :
    {
        bss = .;
        *(.bss)
    }
    . = ALIGN(0x1000); /* align to page size */
    _bss_end = .;
    _bss_size = _bss_end - _bss_start;

    . = 0x70000;
    _stack_start = .;
    
    . = 0x80000;
    _kernel_start = .;

    // Calculate the size of the bootloader by subtracting the start of the text section (_text_start) from the end of the BSS section (_bss_end).
    // The bootloader includes the text, data, and BSS sections, hence the calculation.
    _bl_size = _bss_end - _text_start; /* size of bootloader */
}w