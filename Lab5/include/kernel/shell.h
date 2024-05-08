#ifndef SHELL_H
#define SHELL_H

#include "kernel/uart.h"
#include "kernel/utils.h"
#include "kernel/mailbox.h"
#include "kernel/reboot.h"
#include "kernel/cpio.h"
#include "kernel/allocator.h"
#include "kernel/timer.h"
#include "kernel/task.h"
#include "kernel/INT.h"
#include "kernel/thread.h"
#include "kernel/syscall.h"
#include "kernel/process.h"
#include "kernel/type.h"

void my_shell();

#endif