#include "sys.h"

.global getpid
getpid:
  mov w8, #SYS_GET_PID_NUMBER
  svc #0
  ret

.global uart_read
uart_read:
  mov w8, #SYS_UART_READ_NUMBER
  svc #0
  ret

.global uart_write
uart_write:
  mov w8, #SYS_UART_WRITE_NUMBER
  svc #0
  ret

.global exec
exec:
  mov w8, #SYS_EXEC_NUMBER
  svc #0
  ret

.global fork
fork:
  mov w8, #SYS_FORK_NUMBER
  svc #0
  ret

.global exit
exit:
  mov w8, #SYS_EXIT_NUMBER
  svc #0
  ret

.global mbox_call
mbox_call:
  mov w8, #SYS_MBOX_CALL_NUMBER
  svc #0
  ret

.global kill
kill:
  mov w8, #SYS_KILL_NUMBER
  svc #0
  ret

.global signal
signal:
  mov w8, #SYS_SIGNAL_NUMBER
  svc #0
  ret

.global sigkill
sigkill:
  mov w8, #SYS_SIGKILL_NUMBER
  svc #0
  ret

.global sig_return
sig_return:
  mov w8, #SYS_SIG_RETURN_NUMBER
  svc #0
  ret
