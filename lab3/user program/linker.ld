SECTIONS
{
    . = 0x80000; /* Set the memory address to 0x80000 (start point)*/

    .text : {   /* Define the .text section, which contains executable code*/
        KEEP(*(.text.boot))   /* Keep all .text.boot sections*/
        *(.text)              /* Keep all other .text sections*/
    }

    .rodata : { *(.rodata) }  /* Define the .rodata section, which contains read-only data*/

    .data : { *(.data) }      /* Define the .data section, which contains initialized data*/

    _end = .;                  /* Define _end symbol as the current memory address*/
}

