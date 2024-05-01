SECTIONS
{
    . = 0x80000;
    .text :   {
        KEEP(*(.text.boot)) 
        *(.text) 
    }
    .rodata :{ 
        *(.rodata) 
    }
    .data : { 
        *(.data) 
    }
    .bss :{
        __bss_start = .;
            *(.bss)
            *(COMMON)
        __bss_end = .;
    }
    .heap :{
        __heap_start = .;
    }
    __kernel_end = .;
}

__bss_size = (__bss_end - __bss_start)>>3;
__kernel_start = 0x80000;