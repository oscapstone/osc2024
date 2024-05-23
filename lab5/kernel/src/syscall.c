#include <kernel/bsp_port/mailbox.h>
#include <kernel/bsp_port/ramfs.h>
#include <kernel/bsp_port/uart.h>
#include <kernel/io.h>
#include <kernel/memory.h>
#include <kernel/sched.h>
#include <lib/stddef.h>
#include <lib/string.h>

void sys_debug_msg(const char* message) {
    print_string("[DEBUG] ");
    print_string(message);
    print_string("\n");
}

int sys_getpid() { return get_current()->pid; }

size_t sys_uartread(char buf[], size_t size) {
    for (size_t i = 0; i < size; i++) {
        buf[i] = uart_recv();
    }
    return size;
}

size_t sys_uartwrite(const char buf[], size_t size) {
    for (size_t i = 0; i < size; i++) {
        uart_send(buf[i]);
    }
    return size;
}

int sys_exec(const char* name, char* argv[]) {
    char* file_contents = ramfs_get_file_contents(name);
    uint32_t file_size = ramfs_get_file_size(name);
    if (file_contents == NULL) {
        return -1;
    }

    char* user_program = kmalloc(file_size);
    memcpy(user_program, file_contents, file_size);

    task_struct_t* current_task = get_current();
    struct pt_regs* cur_regs = task_pt_regs(current_task);
    cur_regs->pc = (unsigned long)user_program;
    cur_regs->sp = get_current()->stack + STACK_SIZE;

    return 0;
}

int sys_fork() {
    unsigned long stack = (unsigned long)kmalloc(STACK_SIZE);
    if ((void*)stack == NULL) return -1;
    print_string("Forking new process with stack at ");
    print_h(stack);
    print_string("\n");
    memset((void*)stack, 0, STACK_SIZE);
    return copy_process(0, 0, 0, stack);
}

void sys_exit(int status) { exit_process(); }

int sys_mbox_call(unsigned char ch, unsigned int* mbox) {
    return mailbox_call(ch, mbox);
}

void sys_kill(int pid) { kill_process(pid); }

void* const sys_call_table[] = {sys_getpid,    sys_uartread, sys_uartwrite,
                                sys_exec,      sys_fork,     sys_exit,
                                sys_mbox_call, sys_kill};
