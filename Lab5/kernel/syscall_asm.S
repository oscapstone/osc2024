.global getpid
getpid:
    mov x8, 0
    svc 0
    ret

.global uartread
uartread:
    mov x8, 1
    svc 1
    ret

.global uartwrite
uartwrite:
    mov x8, 2
    svc 2
    ret

.global exec
exec:
    mov x8, 3
    svc 3
    ret

.global fork
fork:
    mov x8, 4
    svc 4
    ret

.global exit
exit:
    mov x8, 5
    svc 5
    ret

.global mbox_call
mbox_call:
    mov x8, 6
    svc 6
    ret

.global kill
kill:
    mov x8, 7
    svc 7
    ret