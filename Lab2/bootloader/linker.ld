__bootloader_start = 0x60000;
__kernel_start = 0x80000;

SECTIONS
{
    . = __kernel_start;
    __begin = .;
    
    .text :   {
        KEEP(*(.text.boot)) 
        *(.text .text.*) 
    }
    .rodata :{ 
        *(.rodata .rodata.*) 
    }
    .data : { *(.data) }
    .bss :
    {
        . = ALIGN(8);

        __bss_start = .;
        *(.bss)
        *(COMMON)
        __bss_end = .;
    }
    __end = .;
    
}

__bss_size = (__bss_end - __bss_start)>>3;
__relocation = 0x60000;