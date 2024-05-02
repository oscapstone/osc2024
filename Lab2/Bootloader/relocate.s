.section ".text.relocate"
.globl _start_relocate

_start_relocate:
    adr x10, .    
    ldr x12, =text_begin  
    adr x13, bss_end

moving_relocate:
    cmp x10, x13            
    b.eq end_relocate          
    ldr x14, [x10], #8      
    str x14, [x12], #8      
    b moving_relocate          

end_relocate:
    ldr x14, =boot_entry    
    br x14

