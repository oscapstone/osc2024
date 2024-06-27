#include "syscall.h"


// thread_start:
//     mov     x29,    0

//     /* Pick the function arg and execute.  */
//     mov	    x0,     x11
//     blr	    x10

//     /* We are done, pass the return value through x0.  */
//     mov	    x8,     #SYS_EXIT_NUMBER
//     svc	    0x0


.globl call_get_pid
call_get_pid:
    mov     w8,     #SYS_CALL_GETPID
    svc     #0
    ret

.globl call_fork
call_fork:
    mov     w8,     #SYS_CALL_FORK
    svc     #0
    ret


.globl call_exit
call_exit:
    mov     w8,     #SYS_CALL_EXIT
    svc     #0
    ret
    