/* see https://blog.louie.lu/2016/11/06/10%E5%88%86%E9%90%98%E8%AE%80%E6%87%82-linker-scripts/ for linker script explaination */

/* set entry point, where the program start to execute first instruction */
ENTRY(_start)

SECTIONS
{
  /* Starts at LOADER_ADDR. */
  /* set current location counter ('.' is location counter) to 0x80000 ,current address is automatically incremented when the linker adds data. */
  /* C stack should start at address 0x80000 and grow downwards, since hardware loads our kernel to address 0x80000 and up, stack can safely run from 0x80000 and down*/
  /* 0x80000 is for aarch64, 0x8000 for 32 bits*/
  /* 0x80000 is our where our .text start*/
  . = 0x80000;

  /* save the current location as '__start'*/
  /* use 'PROVIDE' to prevent double declaration error(already defined in C)*/
  /*PROVIDE(__start = .);*/
  /*__start = .*/

  /* define output sections '.text' */
  /* stuffs in the curly bracket are the content to put into .text output section */
  /*PROVIDE(__text_start = .);*/
  .text : { 
    /* 'KEEP' prevents from link-time garbage collection */
    /* In our case ".text.boot" is to be placed first followed by the more general ".text". ".text.boot" is only used in boot.S and ensures that it ends up at the beginning of the kernel image.  */
    KEEP(*(.text.boot))
    /* Here, we put '.text input section' of all input files in it, as '*' will match all files ends with .text*/
    *(.text .text.* .gnu.linkonce.t*)
  }
  /*PROVIDE(__text_end = .);

  PROVIDE(__rodata_start = .);*/
  /* read only data; it is where global constants shared between processes. */
  .rodata : {
    *(.rodata .rodata.* .gnu.linkonce.r*)
  }
  /*PROVIDE(__rodata_end = .);

  PROVIDE(__data_start = .);*/
  /* global initialized(not 0) non-const variable*/
  .data : {
    *(.data .data.* .gnu.linkonce.d*)
  }
  /*PROVIDE(__data_end = .);*/
  
  
  /*PROVIDE(__bss_start = .);*/
  /* uninitialized global variable. (NOLOAD):link the program properly but must not be loaded to memory, since bss are all 0*/
  /* but in ELF, those initialized to 0 will be in .bss, uninitialized variables will be in COMMON section*/
  /* We put COMMON in .bss, so COMMON will be clear to 0 when program starts*/
  .bss (NOLOAD) : {
    /* align the section so that it starts at an address that is a multiple of 8. If the section is not aligned, it would be more difficult to use the str instruction to store 0*/
    . = ALIGN(16);
    __bss_start = .;
    *(.bss .bss.*) 
    *(COMMON)
    __bss_end = .;
  }
  /*PROVIDE(__bss_end = .);*/

  /*‘/DISCARD/’ may be used to discard input sections. Any input sections which are assigned to an output section named ‘/DISCARD/’ are not included in the output file.*/
  /DISCARD/ : {
    *(.comment)
  }
  __end = .;
  /*PROVIDE(__end = .);*/
}
/*calculate .bss by byte*/
/*PROVIDE(__bss_size = (__bss_end - __bss_start) >> 3);*/
__bss_size = (__bss_end - __bss_start) >> 3;