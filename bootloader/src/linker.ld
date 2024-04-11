/* The physical address at which the the kernel binary will be loaded by the Raspberry's firmware */
_code_start = 0x60000;
_bss_start      = 0x20000000;
_bss_end      = 0x20001000;


ENTRY(_code_start)

/* Flags:
 *     4 == R
 *     5 == RX
 *     6 == RW
 */
PHDRS
{
    segment_code PT_LOAD FLAGS(5);
}

SECTIONS
{
    . =  _code_start;

    /***********************************************************************************************
    * Code
    ***********************************************************************************************/
    .text : {
        _text_start = .;
        KEEP(*(.text.*))
        _text_end = .;
    } :segment_code

    .bss : {
        . = _bss_start;
        *(.bss)
        . = _bss_end;
    }
}

_text_size = _text_end - _text_start;