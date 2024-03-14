SECTIONS
{
	. = 0x80000; 			 /* ensure the correct address*/
	.text.boot : { *(.text.boot) }
	.text : { *(.text) } 	 /*code*/
	.rodata : { *(.rodata) } /* read-only data, like constants */
	.data : { *(.data) } 	 /* initialized data */
	. = ALIGN(0x8);			 /* Align the current location counter (.) to an 8-byte boundar */
	bss_begin = .;			 /* Set the symbol bss_begin to the current location counter (.) */
	.bss : { *(.bss*) } 	 /* uninitialized data */
	bss_end = .;			 /* Set the symbol bss_end to the current location counter (.) */
}