SECTIONS
{
    /* bootloader loaded address in the SD card load address */
    . = 0x60000;
    PROVIDE(__bootloader_start = .);
	.text : { 
        KEEP(*(.text.boot)) 
        *(.text .text.*)
    } /* code section */
	.rodata : { *(.rodata .rodata.*) }
	PROVIDE(_data = .);
	.data : { *(.data .data.*) }
    .bss (NOLOAD) : {
        . = ALIGN(16);
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    }
    __bootloader_end = .;

   /DISCARD/ : { *(.comment) *(.gnu*) *(.note*) *(.eh_frame*) }
}
__bss_size = (__bss_end - __bss_start)>>3;      /* 8 bytes each */
__bootloader_size = (__bootloader_end - __bootloader_start) >> 3;
