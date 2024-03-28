/*
*   Memory structure
*   |    stack    |
*   |      v      |
*   |      ^      |
*   |    heap     |
*   |    bss      | unintialized static data, filled with 0
*   |    data     | initailized variable
*   |    text     | code
*/

SECTIONS
{
    . = 0x80000;
    .text.boot : { KEEP(*(.text.boot)) }
    .text :  { *(.text) }
    

    .rodata : { *(.rodata) }
    .data : { *(.data) }

    /* record start/end of bss to fill 0 */

    . = ALIGN(0x8);
    bss_begin = .;
    .bss (NOLOAD): { *(.bss*) } 
    bss_end = .;
    _end = .;
}