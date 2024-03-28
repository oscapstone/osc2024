
SECTIONS
{
    . = 0x60000;
    
    PROVIDE(_code = .);

    .text.boot : 	{ *(.text.boot) }
    .text :  		{ *(.text) }
    .rodata : 		{ *(.rodata) }
    .data : 		{ *(.data) }
    
    . = ALIGN(0x8);
    
    _bss_begin = .;
    .bss : 			{ *(.bss*) } 
    _bss_end = .;  

    _end = .;

    /DISCARD/ : { *(.comment) *(.gnu*) *(.note*) *(.en_frame*) }
}

_bss_size = (_bss_end -  _bss_begin) >> 3;
_bootloader_size = (_end - _code) >> 3;