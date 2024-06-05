#include "uart.h"
#include "thread.h"
#include "shell.h"
#include "dtb.h"
#include "utils.h"
#include "mbox.h"
#include "vfs.h"
#include "initramfs.h"
#include "tmpfs.h"
#include "syscall.h"

// extern void enable_irq();
// extern void disable_irq();

//get elsel1, elrel1 and sp (Trapframe * = sp)
extern thread * thread_pool[];
extern thread * get_current();
extern void fork_return();

struct trapframe {
    unsigned long x[32]; // x0-x30, 16bytes align (x[31] dummy)
	unsigned long spsr_el1;
	unsigned long elr_el1;
	unsigned long sp_el0;
};

typedef struct trapframe trapframe;

void getpid(trapframe * sp){
    sp -> x[0] =  get_current() -> pid;
}

void exec(char * prog_name){
    struct vnode * node;
    uart_puts(prog_name);
    newline();
    if(vfs_lookup(prog_name, &node)!=0){
        uart_puts("File not found");
        return;
    }

    struct initramfs_node * inode = node -> internal;
    
    int sz = inode -> size;
    char * current = inode -> data;
    //copy program to allocated place
    char *new_program_pos = (char *)allocate_page(sz);
    for (int i = 0; i < sz; i++) {
        *(new_program_pos+i) = *(current+i);
    }
    uart_puts("found user program\n");

    // current is the file address
    asm volatile ("mov x0, 0x340"); //switch to el0 with interrupt enabled
    asm volatile ("msr spsr_el1, x0");
    asm volatile ("msr elr_el1, %0": :"r" (new_program_pos)); // eret address
    asm volatile ("mov x0, %0": : "r"(get_current() -> sp_el0));// user space stack
    asm volatile ("msr sp_el0, x0");
    asm volatile ("eret");
}

void sys_exec(trapframe * sp){
    char * prog = (char *) sp -> x[0];
    exec(prog);
    sp -> x[0] = 0;
}

void exit(trapframe * sp){
    int status = sp -> x[0]; //no use
    thread_exit(); //simply go to exit

    /* 
    //ensure eret version:
    thread * cur = get_current();
    cur -> state = -1; //end
    cur -> priority = 999;
    update_min_priority();
    */
}

void kill(trapframe * sp){
    //make state of pid to -1
    int pid = sp -> x[0];
    thread_pool[pid] -> state = -1;
    thread_pool[pid] -> priority = 999;
    update_min_priority();
}

void uartread(trapframe *sp) {
    enable_irq(); //blocking read, enable irq for preempt
    
    char *buf = (char *)sp->x[0];
    int size = (int)sp->x[1];
    for (int i = 0; i < size; i++) {
        *(buf + i) = uart_getc();
    }
    sp->x[0] = size;
}

void uartwrite(trapframe *sp) {
    char *buf = (char *)sp->x[0];
    int size = (int)sp->x[1];
    for (int i = 0; i < size; i++) {
        uart_send(*(buf + i));
    }
    sp->x[0] = size;
}

void fork(trapframe *sp){
    int pid;
    thread * t = allocate_page(sizeof(thread));
    for(int i=0; i< 64; i++){
        if(thread_pool[i] == 0){
            pid = i;
            uart_puts("Create thread with PID ");
            uart_int(i);
            newline();
            break;
        }
    }
    // memset 0 for the thread
    for(int i = 0; i< sizeof(thread); i++){
        ((char*)t)[i] = 0; 
    }

    t -> pid = pid;
    t -> parent = get_current() -> pid;
    t -> state = get_current() -> state;
    t -> priority = get_current() -> priority;
    t -> sp_el1 = ((unsigned long)allocate_page(4096)) + 4096;// sp start from bottom
    t -> sp_el0 = ((unsigned long)allocate_page(4096)) + 4096;
    t -> preempt = 1;
    // t -> funct = fork_return;

    // copy callee registers
    for(int i = 0; i< sizeof(struct registers); i++){
        ((char*)(&(t -> regs)))[i] = ((char*)(&(get_current() -> regs)))[i]; 
    }
    
    t -> regs.lr = fork_return; //need to load all and eret for child trapframe

    //update user and kernel stack pointer
    unsigned long sp_offset = (char *)(get_current() -> sp_el1) - (char*)sp; //bottom of stack minus top of trapframe(top of stack)
    unsigned long sp_el0_offset = (char *)get_current()->sp_el0 - (char *)sp->sp_el0; //bottom of stack minus top of user stack saved by trapframe

    //copy both stacks
    for(int i = 1; i<= sp_offset; i++){
        *((char *)(t -> sp_el1 - i)) = *((char *)(get_current() -> sp_el1 - i));
    }
   
    for(int i = 1; i <= sp_el0_offset; i++){
        *((char *)(t -> sp_el0 - i)) = *((char *)(get_current() -> sp_el0 - i));
    }

    // modify sp for load all
    t -> regs.sp = t -> sp_el1 - sp_offset; //set to the same position: top of the trapframe
    ((trapframe*)(t -> regs.sp)) -> sp_el0 = t -> sp_el0 - sp_el0_offset; //user stack point to the same position (copied trapframe)

    ((trapframe*)(t -> regs.sp)) -> x[0] = 0; //return 0 for child trapframe
    thread_pool[pid] = t;
    update_min_priority();
    sp -> x[0] = pid;
}

void sys_mbox_call(trapframe * sp){
    unsigned char ch = (unsigned char) sp -> x[0];
    unsigned int * mbox = (unsigned int *) sp -> x[1];
    //lab1 mbox call
    sp -> x[0] = mbox_call(ch, mbox);
}

void open(trapframe * sp){
    // need modify file path
    char * tp = sp -> x[0];
    const char * pathname = path_convert(tp, get_current()->work_dir);
    int flags = sp -> x[1]; 
    int idx = -1;
    for(int i = 0; i < 16; i++){
        if(get_current()->file_table[i] == 0){
            idx = i;
            break;
        }
    }

    if(idx == -1){
        sp -> x[0] = -1;
        return;
    }

    struct file * f;
    int ret = vfs_open(pathname, flags, &f);
    get_current() -> file_table[idx] = f;
    sp -> x[0] = idx;
}

void close(trapframe * sp){
    int fd = sp -> x[0];
    if(get_current() -> file_table[fd] == 0){
        sp -> x[0] = -1;
        uart_puts("No such file!\n\r");
        return;
    }

    vfs_close(get_current()->file_table[fd]);
    get_current() -> file_table[fd] = 0;
    sp -> x[0] = 0;
}

void write(trapframe * sp){
    int fd = sp -> x[0];
    char * buf = sp -> x[1];
    unsigned long count = sp -> x[2];
    if(get_current() -> file_table[fd] == 0){
        sp -> x[0] = -1;
        uart_puts("No such file!\n\r");
        return;
    }
    sp->x[0] = vfs_write(get_current() -> file_table[fd], buf, count);
}

void read(trapframe * sp){
    int fd = sp -> x[0];
    char * buf = sp -> x[1];
    unsigned long count = sp -> x[2];
    if(get_current() -> file_table[fd] == 0){
        sp -> x[0] = -1;
        uart_puts("No such file!\n\r");
        return;
    }
    sp->x[0]= vfs_read(get_current() -> file_table[fd], buf, count);
}


void mkdir(trapframe * sp){
    char * tp = sp -> x[0];
    const char * pathname = path_convert(tp, get_current()->work_dir);
    sp -> x[0] = vfs_mkdir(pathname); 
}

void mount(trapframe * sp){
    char * src = sp -> x[0];
    char * tp = sp -> x[1];
    const char * target = path_convert(tp, get_current()->work_dir);
    char * fs = sp -> x[2];
    sp -> x[0] = vfs_mount(target, fs);
}

void chdir(trapframe * sp){
    char * tp = sp -> x[0];
    const char * ndir = path_convert(tp, get_current() -> work_dir);
    strcpy(ndir, get_current()->work_dir);
    sp -> x[0] = 0;
}

void sys_call(trapframe * sp){
    unsigned long num = sp -> x[8];
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
            sys_exec(sp);
            break;
        case 4:
            fork(sp);
            break;
        case 5:
            exit(sp);
            break;
        case 6:
            sys_mbox_call(sp);
            break;
        case 7:
            kill(sp);
            break;
        case 11:
            open(sp);
            break;
        case 12:
            close(sp);
            break;
        case 13:
            write(sp);
            break;
        case 14:
            read(sp);
            break;
        case 15:
            mkdir(sp);
            break;
        case 16:
            mount(sp);
            break;
        case 17:
            chdir(sp);
            break;
        default:
            uart_puts("Not Implemeted!\n\r");
            break;
    }
}

void sync_exception_entry(unsigned long esr_el1, unsigned long elr_el1, trapframe* sp){
    //sp is the place of "save_all"
    //x0, x1 here is the new ones for function input
    int ec = (esr_el1 >> 26) & 0b111111; //see 31:26 (esr_el1 spec)

    if (ec == 0b010101) { //system call see lab3
        sys_call(sp);
    }
    else{
        /*
        just for debug (lab3 exception handler)
        0b100101
        Data Abort taken without a change in Exception level.
        Used for MMU faults generated by data accesses, alignment faults 
        other than those caused by Stack Pointer misalignment, 
        and synchronous External aborts, including synchronous 
        parity or ECC errors. Not used for debug-related exceptions.
        */
        split_line();
        uart_puts("In exception\n");
        unsigned long spsrel1, elrel1, esrel1;
        asm volatile ("mrs %0, SPSR_EL1" : "=r" (spsrel1));
        uart_puts("SPSR_EL1: 0x");
        uart_hex_long(spsrel1);
        uart_puts("\n");
        asm volatile ("mrs %0, ELR_EL1" : "=r" (elrel1));
        uart_puts("ELR_EL1: 0x");
        uart_hex_long(elrel1);
        uart_puts("\n");
        asm volatile ("mrs %0, ESR_EL1" : "=r" (esrel1));
        uart_puts("ESR_EL1: 0x");
        uart_hex_long(esrel1);
        uart_puts("\n");
        while(1){};
    }
}

void timer_scheduler(){
    unsigned long cntfrq_el0;
    asm volatile ("mrs %0, cntfrq_el0":"=r" (cntfrq_el0));
    asm volatile ("lsr %0, %0, #5":"=r" (cntfrq_el0) :"r"(cntfrq_el0)); // 1/32 second tick
    asm volatile ("msr cntp_tval_el0, %0" : : "r"(cntfrq_el0));
    schedule();
}