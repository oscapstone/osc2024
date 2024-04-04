.globl irq_vector_init
irq_vector_init:
    adr x0, exception_vector_table
    msr vbar_el1, x0
    ret
