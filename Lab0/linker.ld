SECTIONS
{
  /* Starts at LOADER_ADDR. */
  /* set current location as 0x8000 ,current address is automatically incremented when the linker adds data. */
  /* C stack should start at address 0x8000 and grow downwards, since hardware loads our kernel to address 0x8000 and up, stack can safely run from 0x8000 and down*/
  /* not quite sure why it's 0x80000 here(or maybe they're just the same)*/
  . = 0x80000;
  /* define output sections '.text' */
  /* stuffs in the curly bracket are the content to put into .text output section */
  .text : { *(.text) }
}