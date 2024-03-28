_heap_stack_size = 1M;

SECTIONS
{
    /* bootloader 0x60000, kernel 0x80000 */
    . = 0x80000;
    _start = .; /* Define the start symbol at the current address */
    .text : { *(.text.boot) *(.text) }
    .rodata : { *(.rodata) }
    .data : { *(.data) }
    .bss : {
        _bss_top = .; /* Mark the start of the .bss section */
        *(.bss)       /* Include all .bss sections from input files */
    }
    _bss_size = SIZEOF(.bss) >> 3;
    .heap : {
        . = ALIGN(8);
        _heap_top = .;
    } 
    /* Increase the location counter by the size of the heap and stack */
    . = . + _heap_stack_size;
    .stack : {
        . = ALIGN(8);
        _stack_top = .; /* Mark the top of the stack (end of the .stack section) */
    }
    /* Define the end symbol at the current address */
    _end = .;

}
