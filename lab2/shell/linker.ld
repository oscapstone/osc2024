SECTIONS
{
    . = 0x80000; /* Set the memory address to 0x80000 (start point)*/

    .text : {   /* Define the .text section, which contains executable code*/
        KEEP(*(.text.boot))   /* Keep all .text.boot sections*/
        *(.text)              /* Keep all other .text sections*/
    }

    .rodata : { *(.rodata) }  /* Define the .rodata section, which contains read-only data*/

    .data : { *(.data) }      /* Define the .data section, which contains initialized data*/

    .bss () : {               /* Define the .bss section, which contains uninitialized data*/
        __bss_start = .;      /* Define __bss_start symbol as the current memory address*/
        *(.bss)               /* Keep all .bss sections*/
        *(COMMON)             /* Keep all common symbols*/
        __bss_end = .;        /* Define __bss_end symbol as the current memory address*/
    }
    __heap_start = .;
    _end = .;                  /* Define _end symbol as the current memory address*/
}

__bss_size = (__bss_end - __bss_start) >> 3; /* Calculate the size of the .bss section in bytes*/
