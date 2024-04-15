SECTIONS
{
  . = 0x80000;
  .text : {
    KEEP(*(.text._start))
    *(.text._start_arguments)
    *(.text*)
  }
}