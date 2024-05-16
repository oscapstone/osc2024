#include "syscall.h"
#include "thread.h"

uint64_t getpid(uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3) {
    return cur_thread->pid;
}