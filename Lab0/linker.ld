SECTIONS
{
  . = 0x80000;
  a : {a.o(.text)}
  .text : { *(.text) }
}
