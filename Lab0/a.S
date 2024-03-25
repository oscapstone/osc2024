#where this code belongs(.text section) in the compiled binary.
.section ".text"    
_start:
  #Wait for event
  #When the C function returns, it enters the halt procedure where it loops forever doing nothing.
  wfe
  b _start