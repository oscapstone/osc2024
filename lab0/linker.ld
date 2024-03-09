SECTIONS
{
	. = 0x80000;
	.text : { KEEP(*(.text.boot)) }
}