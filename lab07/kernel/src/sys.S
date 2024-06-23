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

.global open
open:
    mov w8, #SYS_OPEN_NUM
    svc #0
    ret

.global close
close:
    mov w8, #SYS_CLOSE_NUM
    svc #0
    ret

.global write
write:
    mov w8, #SYS_WRITE_NUM
    svc #0
    ret

.global read
read:
    mov w8, #SYS_READ_NUM
    svc #0
    ret

.global mkdir
mkdir:
    mov w8, #SYS_MKDIR_NUM
    svc #0
    ret

.global mount
mount:
    mov w8, #SYS_MOUNT_NUM
    svc #0
    ret

.global chdir
chdir:
    mov w8, #SYS_CHDIR_NUM
    svc #0
    ret

.global lseek64
lseek64:
    mov w8, #SYS_LSEEK64_NUM
    svc #0
    ret


.global ioctl
ioctl:
    mov w8, #SYS_IOCTL_NUM
    svc #0
    ret