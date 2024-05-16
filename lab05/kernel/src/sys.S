#include "sys.h"

// reason for using w8 is that x0-x7 are used for passing arguments
// the such warpper functions are not inlcuded in the real linux kernel
// they often are packed in the different language's standard library, such as glibc

// svc #0 is used to generate synchronous exception

.global getpid
getpid:
    mov w8, #SYS_GETPID_NUM
    svc #0  // using svc to generate synchronous exception
    ret

.global uartread
uartread:
    mov w8, #SYS_UARTREAD_NUM
    svc #0
    ret

.global uartwrite
uartwrite:
    mov w8, #SYS_UARTWRITE_NUM
    svc #0
    ret

.global exec
exec:
    mov w8, #SYS_EXEC_NUM
    svc #0
    ret

.global fork
fork:
    mov w8, #SYS_FORK_NUM
    svc #0
    ret

.global exit
exit:
    mov w8, #SYS_EXIT_NUM
    svc #0
    ret

.global mbox_call
mbox_call:
    mov w8, #SYS_MBOX_CALL_NUM
    svc #0
    ret

.global kill
kill:
    mov w8, #SYS_KILL_NUM
    svc #0
    ret
