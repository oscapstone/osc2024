exception_handler:
  bl  print_el1_sys_reg
  eret


.align 11                       // vector table should be aligned to 0x800 (2^11)
.global exception_vector_table
exception_vector_table:
  b exception_handler           // branch to a handler function.
  .align 7                      // entry size is 0x80, .align will pad 0
  b exception_handler
  .align 7
  b exception_handler
  .align 7
  b exception_handler
  .align 7

  b exception_handler
  .align 7
  b exception_handler
  .align 7
  b exception_handler
  .align 7
  b exception_handler
  .align 7

  b exception_handler
  .align 7
  b exception_handler
  .align 7
  b exception_handler
  .align 7
  b exception_handler
  .align 7

  b exception_handler
  .align 7
  b exception_handler
  .align 7
  b exception_handler
  .align 7
  b exception_handler
  .align 7

.global set_exception_vector_table			//use as function
set_exception_vector_table:
  adr x0, exception_vector_table
  msr vbar_el1, x0                // Vector Base Address Register for EL1
  ret