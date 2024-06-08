#include "syscall.h"

#define SYS_GET_PID 0
#define SYS_UART_READ 1
#define SYS_UART_WRITE 2
#define SYS_EXEC 3
#define SYS_FORK 4
#define SYS_EXIT 5
#define SYS_MBOX_CALL 6
#define SYS_KILL 7
#define SYS_SIGNAL 8
#define SYS_SIGNAL_KILL 9
#define SYS_MMAP 10

#define SYS_SIGRETURN 50

.global getpid
getpid:
    mov x8, SYS_GET_PID
    svc #0
    ret

.global uartread
uartread:
    mov x8, SYS_UART_READ
    svc #0
    ret

.global uartwrite
uartwrite:
    mov x8, SYS_UART_WRITE
    svc #0
    ret

.global exec
exec:
    mov x8, SYS_EXEC
    svc #0
    ret

.global fork
fork:
    mov x8, SYS_FORK
    svc #0
    ret

.global exit
exit:
    mov x8, SYS_EXIT
    svc #0
    b halt

.global mbox_call
mbox_call:
    mov x8, SYS_MBOX_CALL
    svc #0
    ret

.global kill
kill:
    mov x8, SYS_KILL
    svc #0
    ret

.global signal
signal:
    mov x8, SYS_SIGNAL
    svc #0
    ret

.global signal_kill
signal_kill:
    mov x8, SYS_SIGNAL_KILL
    svc #0
    ret

.global mmap
mmap:
    mov x8, SYS_MMAP
    svc #0
    ret

.global sigreturn
sigreturn:
    mov x8, SYS_SIGRETURN
    svc #0
    ret