.globl get_el_TYPE
get_el_TYPE:
   mrs x0, CurrentEl
   lsr x0, x0, #2
   ret

