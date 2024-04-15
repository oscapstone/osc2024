.globl irq_vector_init
irq_vector_init:
    adr x0, exception_vector_table
    msr vbar_el1, x0
    ret

.globl enable_fiq
enable_fiq:
    msr daifclr, #1
    ret

.globl disable_fiq
disable_fiq:
    msr daifset, #1
    ret

.globl enable_irq
enable_irq:
    msr daifclr, #2
    ret

.globl disable_irq
disable_irq:
    msr daifset, #2
    ret

.globl enable_serror
enable_serror:
    msr daifclr, #4
    ret

.globl disable_serror
disable_serror:
    msr daifset, #4
    ret

.globl enable_debug
enable_debug:
    msr daifclr, #8
    ret

.globl disable_debug
disable_debug:
    msr daifset, #8
    ret

.globl enable_all_exception
enable_all_exception:
    msr daifclr, #0xf
    ret

.globl disable_all_exception
disable_all_exception:
    msr daifset, #0xf
    ret
