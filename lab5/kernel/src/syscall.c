#include <kernel/bsp_port/mailbox.h>
#include <kernel/bsp_port/ramfs.h>
#include <kernel/bsp_port/uart.h>
#include <kernel/io.h>
#include <kernel/memory.h>
#include <kernel/sched.h>
#include <lib/stddef.h>
#include <lib/string.h>

void sys_debug_msg(const char* message) {
    print_string("[Sys DEBUG] ");
    print_string(message);
    print_string("\n");
}

int sys_getpid() {
    char buf[100];
    sprintf(buf, "Getting PID: %d\n", get_current()->pid);
    // sys_debug_msg(buf);
    // print_d(get_current()->pid);
    return get_current()->pid;
}

size_t sys_uartread(char buf[], size_t size) {
    // sys_debug_msg("Reading from UART");
    for (size_t i = 0; i < size; i++) {
        buf[i] = uart_recv();
    }
    return size;
}

size_t sys_uartwrite(const char buf[], size_t size) {
    // sys_debug_msg("Writing to UART");
    for (size_t i = 0; i < size; i++) {
        uart_send(buf[i]);
    }
    return size;
}

int sys_exec(const char* name, char* argv[]) {
    // sys_debug_msg("Executing program: ");
    unsigned long file_size = ramfs_get_file_size(name);

    if (file_size == 0) {
        return -1;
    }

    void* user_program = kmalloc(file_size + STACK_SIZE); // make a stack at the end of the program
    if (user_program == NULL) {
        return -1;
    }
    ramfs_get_file_contents(name, user_program);
    memset(user_program + file_size, 0, STACK_SIZE);

    preempt_disable();
    task_struct_t* current_task = get_current();
    struct pt_regs* cur_regs = task_pt_regs(current_task);
    cur_regs->pc = (unsigned long)user_program;
    cur_regs->sp = current_task->stack + STACK_SIZE;
    preempt_enable();

    return 0;
}

int sys_fork() {
    // print_string("Forking new process\n");
    unsigned long stack = (unsigned long)kmalloc(STACK_SIZE);
    if ((void*)stack == NULL) return -1;
    // print_string("Forking new process with stack at ");
    // print_h(stack);
    // print_string("\n");
    memset((void*)stack, 0, STACK_SIZE);

    return copy_process(0, 0, 0, stack);
}

void sys_exit(int status) {
    sys_debug_msg("Exiting process");
    exit_process();
}

int sys_mbox_call(unsigned char ch, unsigned int* mbox) {
    return mailbox_call(ch, mbox);
}

void sys_kill(int pid) { kill_process(pid); }

void* const sys_call_table[] = {sys_getpid,    sys_uartread, sys_uartwrite,
                                sys_exec,      sys_fork,     sys_exit,
                                sys_mbox_call, sys_kill};
