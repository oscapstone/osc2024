#include "uart.h"
#include "thread.h"
#include "shell.h"
#include "dtb.h"

//get elsel1, elrel1 and sp (Trapframe * = sp)
//typedef struct thread thread;
extern thread * thread_pool[];
extern thread * get_current();

struct trapframe {
    unsigned long x[31]; // general register from x0 ~ x30
    //unsigned long sp_el0;
    unsigned long elr_el1;
    unsigned long spsr_el1;
};

typedef struct trapframe trapframe;

void getpid(trapframe * sp){
    sp -> x[0] =  get_current() -> pid;//get_pid();
    // uart_puts("Current Thread: ");
    // uart_int(sp -> x[0]);
    // newline();
}

void exec(trapframe * sp){
    char * prog = (char *) sp -> x[0];
    struct cpio_newc_header *fs = (struct cpio_newc_header *)cpio_base ;
    char *current = (char *)cpio_base ;
    int sz;
    while (1) {
        fs = (struct cpio_newc_header *)current;
        int name_size = hex_to_int(fs->c_namesize, 8);
        int file_size = hex_to_int(fs->c_filesize, 8);
        sz = file_size;
        current += 110; // size of cpio_newc_header
        
        if (strcmp(current, prog) == 0){
            current += name_size;
            if((current - (char *)fs) % 4 != 0)
                current += (4 - (current - (char *)fs) % 4);
            break;
        }

        current += name_size;
        if((current - (char *)fs) % 4 != 0)
            current += (4 - (current - (char *)fs) % 4);
        
        current += file_size;
        if((current - (char *)fs) % 4 != 0)
            current += (4 - (current - (char *)fs) % 4);
    }
    uart_puts("found user program\n");
    //uart_int(sz);
    char *new_program_pos = (char *)allocate_page(sz);
    for (int i = 0; i < sz; i++) {
        //hi();
        *(new_program_pos+i) = *(current+i);
    }
    // for (int i = 0; i < sz; i++) {
    //     if(*(new_program_pos+i) != *(current+i)){
    //         uart_int("BUG!!!!!!!\n");
    //     }
    // }
    int r = 100;
        while(r--) { asm volatile("nop"); }
    // current is the file address
    asm volatile ("mov x0, 0"); 
    asm volatile ("msr spsr_el1, x0");
    asm volatile ("msr elr_el1, %0": :"r" (new_program_pos));
    asm volatile ("mov x0, %0": : "r"(get_current() -> sp_el0));
    asm volatile ("msr sp_el0, x0");
    asm volatile ("eret");
    sp -> x[0] = 0;
}

void exit(trapframe * sp){
    int status = sp -> x[0];
    thread_exit();
}

void kill(trapframe * sp){
    int pid = sp -> x[0];
    thread_pool[pid] -> state = -1;
    thread_pool[pid] -> priority = 999;
    update_min_priority();
}

void uartread(trapframe *trapframe) {
    char *buf = (char *)trapframe->x[0];
    int size = (int)trapframe->x[1];
    for (int i = 0; i < size; i++) {
        *(buf + i) = uart_getc();
    }
    trapframe->x[0] = size;
}

void uartwrite(trapframe *trapframe) {
    const char *buf = (const char *)trapframe->x[0];
    int size = (int)trapframe->x[1];
    for (int i = 0; i < size; i++) {
        uart_send(*(buf + i));
    }
    trapframe->x[0] = size;
}

void sys_call(trapframe * sp){
    unsigned long num = sp -> x[8];
    // uart_puts("System Call: ");
    // uart_int(num);
    // newline();
    switch (num){
        case 0:
            getpid(sp);
            break;
        case 1:
            uartread(sp);
            break;
        case 2:
            uartwrite(sp);
            break;
        case 3:
            exec(sp);
            break;
        case 5:
            exit(sp);
            break;
        case 7:
            kill(sp);
            break;
    }
}

void sync_exception_entry(unsigned long esr_el1, unsigned long elr_el1, trapframe* sp){
    int ec = (esr_el1 >> 26) & 0b111111;
    //sp is the place of "save_all"
    //x0, x1 here is the new for function input

    if (ec == 0b010101) { //system call
        sys_call(sp);
    }
}