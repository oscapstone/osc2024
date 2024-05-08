#include "syscall.h"

.global get_taskid
get_taskid:
    mov x8, SYS_GET_TASKID
    svc 0
    ret

.global uart_read
uart_read:
    mov x8, SYS_UART_READ
    svc 0
    ret

.global uart_write
uart_write:
    mov x8, SYS_UART_WRITE
    svc 0
    ret

.global exec
exec:
    mov x8, SYS_EXEC
    svc 0
    ret

.global fork
fork:
    mov x8, SYS_FORK
    svc 0
    ret

.global exit
exit:
    mov x8, SYS_EXIT
    svc 0
    ret

.global mailbox_call
mailbox_call:
    mov x8, SYS_MBOX_CALL
    svc 0
    ret

.global kill
kill:
    mov x8, SYS_KILL
    svc 0
    ret

.global sigreturn
sigreturn:
    mov x8, SYS_SIGRETURN
    svc 0
    ret