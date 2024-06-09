.globl getpid
getpid:
    mov x8, #0
    svc 0
    ret

.globl uart_read
uart_read:
    mov x8, #1
    svc 0
    ret

.globl uart_write
uart_write:
    mov x8, #2
    svc 0
    ret

.globl exec
exec:
    mov x8, #3
    svc 0
    ret 

.globl fork
fork:
    mov x8, #4
    svc 0
    ret

.globl exit
exit:
    mov x8, #5
    svc 0
    ret 

.globl mbox_call_doge
mbox_call_doge:
    mov x8, #6
    svc 0
    ret 

.globl kill
kill:
    mov x8, #7
    svc 0
    ret 