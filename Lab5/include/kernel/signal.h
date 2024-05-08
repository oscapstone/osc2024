#ifndef SIGNAL_H
#define SIGNAL_H

#include "kernel/uart.h"
#include "kernel/process.h"
#include "kernel/allocator.h"
#include "kernel/syscall.h"
#include "kernel/lock.h"

extern void store_context(void *context);

void signal_default_handler();
void check_signal(trap_frame_t *tf);
void run_signal(trap_frame_t *tf, int signal);
void signal_handler_wrapper();

#endif