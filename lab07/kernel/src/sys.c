#include "sys.h"
#include "schedule.h"
#include "fork.h"
#include "mini_uart.h"
#include "io.h"
#include "alloc.h"
#include "cpio.h"
#include "lib.h"
#include "type.h"
#include "string.h"
#include "peripherals/mailbox.h"

extern struct task_struct *current;
extern void memzero_asm(unsigned long src, unsigned long n);
// #ifndef QEMU
// extern unsigned long CPIO_START_ADDR_FROM_DT;
// #endif

int sys_getpid()
{
    return current->pid;
}

size_t sys_uartread(char buf[], size_t size)
{
    for(size_t i=0; i<size; i++){
        buf[i] = uart_recv();
    }
    return size;
}

size_t sys_uartwrite(const char buf[], size_t size)
{
    for(size_t i=0; i<size; i++){
        uart_send(buf[i]);
    }
    return size;
}

int sys_exec(const char *name, char *const argv[]) // [TODO]
{
#ifndef QEMU
    cpio_newc_header* head = (void*)(uint64_t)CPIO_START_ADDR_FROM_DT;
#else
    cpio_newc_header* head = (void*)(uint64_t)CPIO_ADDR;
#endif
    uint32_t head_size = sizeof(cpio_newc_header);

    while(1)
    {
        int namesize = strtol(head->c_namesize, 16, 8);
        int filesize = strtol(head->c_filesize, 16, 8);

        char *filename = (void*)head + head_size;

        uint32_t offset = head_size + namesize;
        if(offset % 4 != 0) offset = ((offset/4 +1)*4);

        if(strcmp(filename, "TRAILER!!!") == 0){
            printf("\r\n[ERROR] File not found");
            break;
        }
        else if(strcmp(filename, name) == 0){
            /* The filedata is appended after filename */
            char *filedata = (void*)head + offset;
            void* user_program_addr = balloc(filesize+THREAD_SIZE); // extra page in case
            if(user_program_addr == NULL) return -1;
            memzero_asm((unsigned long)user_program_addr, filesize+THREAD_SIZE);
            for(int i=0; i<filesize; i++){
                ((char*)user_program_addr)[i] = filedata[i];
            }

            preempt_disable(); // leads to get the correct current task

            struct pt_regs *cur_regs = task_pt_regs(current);
            cur_regs->pc = (unsigned long)user_program_addr;
            cur_regs->sp = current->stack + THREAD_SIZE;

            preempt_enable();
            break;  // won't reach here
        }

        if(filesize % 4 != 0) filesize = (filesize/4 +1)*4;
        head = (void*)head + offset + filesize;
    }
    return -1; // only failure
}

int sys_fork() // [TODO]
{
    unsigned long stack = (unsigned long)balloc(THREAD_SIZE);
    if((void*)stack == NULL) return -1;
    memzero_asm(stack, THREAD_SIZE);
    return copy_process(0, 0, 0, stack);
}

void sys_exit(int status) // [TODO]
{
    exit_process();
}

int sys_mbox_call(unsigned char ch, unsigned int *mbox) // [TODO]
{
    unsigned int mesg = (((unsigned int)(unsigned long)mbox) & ~0xf) | (ch & 0xf);
    while(*MAILBOX_STATUS & MAILBOX_FULL){   // // Check if Mailbox 0 status register’s full flag is set. if MAILBOX_STATUS == 0x80000001, then error parsing request buffer 
        asm volatile("nop");
    }

    *MAILBOX_WRITE = mesg;

    while(1){
        while(*MAILBOX_STATUS & MAILBOX_EMPTY){
            asm volatile("nop");
        }
        if(mesg == *MAILBOX_READ){
            return mbox[1] == REQUEST_SUCCEED;
        }
    }
    return 0;
}

void sys_kill(int pid) // [TODO]
{
    struct task_struct* p;
    for(int i=0; i<NR_TASKS; i++){
        if(task[i] == NULL) continue; // (task[i] == NULL) means no more tasks
        p = task[i];
        if(p->pid == pid){
            preempt_disable();
            printf("\r\nKilling process: "); printf_int(pid);
            p->state = TASK_ZOMBIE;
            preempt_enable();
            return;
        }
    }
    printf("\r\nProcess not found: "); printf_int(pid);
    return;
}

void * const sys_call_table[] = {sys_getpid, sys_uartread, sys_uartwrite, sys_exec, sys_fork, sys_exit, sys_mbox_call, sys_kill};