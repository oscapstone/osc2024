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
    .relocate :
    { 
        KEEP(*(.text.relocate))
    }

    . = ALIGN(4096);
    _begin = .;
    .text :
    { 
        KEEP(*(.text.boot))
        *(.text)
    }
    

    .rodata : { *(.rodata) }
    .data : { *(.data) }

    . = ALIGN(0x8);
    bss_begin = .;
    .bss (NOLOAD): { *(.bss*) } 
    bss_end = .;
    _end = .;
}
_bootloader = 0x60000;
