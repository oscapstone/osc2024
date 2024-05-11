#include "syscall.h"

#define SYS_GET_PID 0
#define SYS_UART_READ 1
#define SYS_UART_WRITE 2
#define SYS_EXEC 3
#define SYS_FORK 4
#define SYS_EXIT 5

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