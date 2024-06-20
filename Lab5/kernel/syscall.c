#include "syscall.h"

extern thread_t* cur_thread;
extern thread_t* thread_arr;

int sys_getpid(){
    return cur_thread->id;
}

uint32_t sys_uart_read(char buf[], uint32_t size){
    uint32_t count = 0;

    for ( ; count < size; count++){
        buf[count] = uart_read();    
    }

    return count;
}

uint32_t sys_uart_write(const char buf[], uint32_t size){
    uint32_t count = 0;

    for ( ; count < size; count++){
        uart_write(buf[count]);
    }

    return count;
}

// Temporarily don't need argv
int sys_exec(const char* name, char* const argv[]){
    char* file_content = get_cpio_file(name);
    int file_size = get_cpio_fsize(name);

    
    disable_interrupt();
    cur_thread->prog_size = file_size;
    memcpy(file_content, cur_thread->prog, file_size);
    enable_interrupt();
    
    return 0;
}

int sys_fork(trap_frame_t* tpf){
    disable_interrupt();

    int child_pid = copy_thread(tpf);
    // print_str("\nchild_pid: ");
    // print_dec(child_pid);

    enable_interrupt();

    // async_uart_puts("\nchild id: ");
    // async_uart_dec(new_thread->id);

    return child_pid;
}

void sys_exit(){
    disable_interrupt();
    // print_str("\nExit Thread ID: ");
    // print_dec(cur_thread->id);
    cur_thread->status = DEAD;
    enable_interrupt();
    schedule();
}

int sys_mbox_call(unsigned char ch, unsigned int* mbox){
    disable_interrupt();

    /* argument mailbox is mailbox address */
    unsigned int msg = ((unsigned long)mbox & ~0xf) | (ch & 0xf);

    // Check if Mailbox 0 status register’s full flag is set.
    while (*MAILBOX_STATUS & MAILBOX_FULL) {
        asm volatile("nop");
    };
    
    // If not full, then you can write to Mailbox 1 Read/Write register.
    *MAILBOX_WRITE = msg;

    while (1) {
        // Check if Mailbox 0 status register’s empty flag is set.
        while (*MAILBOX_STATUS & MAILBOX_EMPTY) {
            asm volatile("nop");
        };

        // If not, then you can read from Mailbox 0 Read/Write register.
        // Check if the value is the same as you wrote in step 1.
        if (msg == *MAILBOX_READ){
            enable_interrupt();
            return mbox[1] == REQUEST_SUCCEED;
        }
    }

    enable_interrupt();

    return 0;
}

void sys_kill(int pid){
    disable_interrupt();

    if (pid < 0 || pid >= MAX_TID || thread_arr[pid].status == FREE){
        enable_interrupt();
        return;
    }

    // print_str("\nHere");
    thread_arr[pid].status = DEAD;
    enable_interrupt();

    return;
}

void signal_register(int SIGNAL, void(*handler)()){
    if (SIGNAL > MAX_SIGNAL || SIGNAL < 0)
        return;

    cur_thread->signal_handler[SIGNAL] = handler;
}

void signal_kill(int pid, int SIGNAL){

    disable_interrupt();
    if (pid < 0 || pid >= MAX_TID || thread_arr[pid].status == FREE){
        enable_interrupt();
        return;
    }

    thread_arr[pid].signal_count[SIGNAL]++;
    // print_str("\nKill PID: ");
    // print_dec(pid);
    // print_str("\nsignal_count: ");
    // print_dec(thread_arr[pid].signal_count[SIGNAL]);
    enable_interrupt();
}

void signal_return(trap_frame_t* tpf){
    free(cur_thread->signal_handler_sp);

    tpf->elr_el1 = cur_thread->sig_ctx.elr_el1;
    tpf->sp_el0 = cur_thread->sig_ctx.sp_el0;
    tpf->x30 = cur_thread->sig_ctx.x30;
    tpf->spsr_el1 = cur_thread->sig_ctx.spsr_el1;

    // enable_interrupt();

    // print_str("\nReturn Address: ");
    // print_hex(cur_thread->ctx.lr);
    // print_str("\nFree signal_sp: ");
    // print_hex(cur_thread->signal_handler_sp);
    
    load_ctx(&cur_thread->ctx);
    // print_str("\nHere");
    // schedule();
}

void fork_test(){

    // EQUAL: printf("\nFork Test, pid %d\n", get_pid());
    print_str("\nFork Test, pid ");
    print_dec(getpid());

    int cnt = 1;
    int ret = 0;

    if ((ret = fork()) == 0) { // child
        long long cur_sp;
        asm volatile("mov %0, sp" : "=r"(cur_sp));
        // printf("first child pid: %d, cnt: %d, ptr: %x, sp : %x\n", sys_getpid(), cnt, &cnt, cur_sp);
        print_str("\nFirst child pid: ");
        print_dec(getpid());
        print_str(", cnt: ");
        print_dec(cnt);
        print_str(", ptr: ");
        print_hex(&cnt);
        print_str(", sp: ");
        print_hex(cur_sp);

        ++cnt;

        if ((ret = fork()) != 0){
            asm volatile("mov %0, sp" : "=r"(cur_sp));
            // printf("first child pid: %d, cnt: %d, ptr: %x, sp : %x\n", sys_getpid(), cnt, &cnt, cur_sp);
            print_str("\nFirst child pid: ");
            print_dec(getpid());
            print_str(", cnt: ");
            print_dec(cnt);
            print_str(", ptr: ");
            print_hex(&cnt);
            print_str(", sp: ");
            print_hex(cur_sp);

            while(1){
                // delay(1000000);
                // print_str("\nAAAA");
            }
        }
        else{
            while (cnt < 5) {
                asm volatile("mov %0, sp" : "=r"(cur_sp));
                // printf("second child pid: %d, cnt: %d, ptr: %x, sp : %x\n", sys_getpid(), cnt, &cnt, cur_sp);
                print_str("\nSecond child pid: ");
                print_dec(getpid());
                print_str(", cnt: ");
                print_dec(cnt);
                print_str(", ptr: ");
                print_hex(&cnt);
                print_str(", sp: ");
                print_hex(cur_sp);

                delay(1000000);
                ++cnt;
            }

            while(1){
                // delay(1000000);
                // print_str("\nBBBB");
            }
        }
        exit();
    }
    else {
        // printf("parent here, pid %d, child %d\n", sys_getpid(), ret);
        print_str("\nParent here, pid: ");
        print_dec(getpid());
        print_str(", child: ");
        print_dec(ret);
        // exit();

        while(1){
            // delay(1000000);
            // print_str("\nCCCC");
        }

    }

}
