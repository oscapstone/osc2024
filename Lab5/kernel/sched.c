#include "sched.h"

thread_t* RQ_head = 0;
thread_t* RQ_tail = 0;

extern thread_t* cur_thread;

void init_scheduler(){
    init_thread_arr();
    thread_t* main_thread = _thread_create(idle);
    set_ctx(&(main_thread->ctx));

    uint64_t main_stack;

    asm volatile(
        "mov    %0, sp"
        : "=r"(main_stack)
    );

    main_thread->kernel_sp = main_stack;
    cur_thread = main_thread;
    cur_thread->status = RUNNING;
    
    thread_t* shell_thread = _thread_create(shell);
    push_rq(shell_thread);

}

void idle(){
    while (1){
        kill_zombies();
        schedule();
    }
}

void schedule_timer(){
    sleep(schedule_timer, 1);
    // sleep(schedule_timer, 5);
    schedule();
}

void schedule(){
    
    if (RQ_head != 0){

        thread_t* off_thread = cur_thread;

        while (1){
            thread_t* target_thread = pop_rq();
            
            if (target_thread->status == DEAD){
                // async_uart_puts("\nDEAD");
                push_rq(target_thread);
            }else{
                cur_thread = target_thread;
                break;
            }
        }

        cur_thread->status = RUNNING;

        if (off_thread->status != DEAD)
            off_thread->status = READY;

        // async_uart_puts("\nOff Thread PID: ");
        // async_uart_dec(off_thread->id);
        // async_uart_puts("\nStatus: ");
        // async_uart_dec(off_thread->status);

        push_rq(off_thread);

        // if (off_thread->status == DEAD)
        //     check_RQ();

        // if (off_thread->id == 2 && off_thread->status == DEAD)
        //     check_RQ();
        
        switch_to(get_current(), &(cur_thread->ctx));

        // check_RQ();
    }

}   



void kill_zombies(){
    thread_t* th_ptr = RQ_head;

    disable_interrupt();
    while (th_ptr != 0){
        if (th_ptr->status == DEAD){

            if (th_ptr == RQ_tail){
                RQ_tail = th_ptr->prev;
            }

            th_ptr->status = FREE;

            free(th_ptr->kernel_sp);
            free(th_ptr->user_sp);

            for (int i = 0; i < MAX_SIGNAL; i++){
                th_ptr->signal_handler[i] = 0;
                th_ptr->signal_count[i] = 0;
            }
            
            if (th_ptr->prog_size > 0){
                free(th_ptr->prog);
                th_ptr->prog_size = 0;
            }
            
            if (th_ptr->prev == 0){
                RQ_head = th_ptr->next;

                if (th_ptr->next != 0)
                    th_ptr->next->prev = 0;
            }else {
                th_ptr->prev->next = th_ptr->next;

                if (th_ptr->next != 0){
                    th_ptr->next->prev = th_ptr->prev;
                }
            }

            // check_RQ();
        }

        th_ptr = th_ptr->next;
    }

    enable_interrupt();
    // check_RQ();
}

void set_ctx(uint64_t ctx_addr){
    asm volatile(
        "msr tpidr_el1, %0;" :: "r"(ctx_addr)
    );
}

void thread_create(void (*func)(void)){
    thread_t* new_thread = _thread_create(func);

    if (new_thread == 0)
        return;

    // async_uart_puts("\nuser thread function: ");
    // async_uart_hex((uint32_t)func);

    push_rq(new_thread);

    // check_RQ();
}

void thread_exec(void* prog, uint32_t prog_size){

    disable_interrupt();
    if (cur_thread->prog_size > 0){
        free(cur_thread->prog);
        cur_thread->prog_size = 0;
    }

    if (prog_size > 0){
        cur_thread->prog_size = prog_size;
        cur_thread->prog = malloc(prog_size);
        memcpy(prog, cur_thread->prog, prog_size);
    }else {
        cur_thread->prog = prog;
    }   
    enable_interrupt();

    exec_in_el0(cur_thread->prog, cur_thread->user_sp + USTACK_SIZE); 
}

void thread_exit(){
    disable_interrupt();
    cur_thread->status = DEAD;
    enable_interrupt();
    schedule();
}

int copy_thread(trap_frame_t* tpf){
    thread_t* child_thread = _thread_create(child_fork_ret);
    
    memcpy(cur_thread->user_sp, child_thread->user_sp, USTACK_SIZE);
    memcpy(cur_thread->kernel_sp, child_thread->kernel_sp , KSTACK_SIZE);
    
    if (cur_thread->prog_size > 0) {
        child_thread->prog_size = cur_thread->prog_size;
        child_thread->prog = (char*)malloc(child_thread->prog_size);
        memcpy(cur_thread->prog, child_thread->prog, cur_thread->prog_size);
    }

    for (int i = 0; i < MAX_SIGNAL; i++)
        child_thread->signal_handler[i] = cur_thread->signal_handler[i];
    

    trap_frame_t* child_tpf = (trap_frame_t*)((uint8_t*)tpf + ((uint8_t*)child_thread->kernel_sp - (uint8_t*)cur_thread->kernel_sp));
    
    child_thread->ctx.sp = child_tpf;

    // print_str("\nParent Kernel Sp: ");
    // print_hex(tpf);
    // print_str("\nChild Kernel Sp: ");
    // print_hex(child_tpf);

    // check x8
    // print_str("\nsyscall_no: ");
    // print_dec(child_tpf->x8);
    // print_str("\nactual syscall_no: ");
    // print_dec(tpf->x8);
    
    

    child_tpf->x0 = 0;
    child_tpf->x29 = child_thread->ctx.fp;
    child_tpf->sp_el0 = tpf->sp_el0 + (uint64_t)child_thread->user_sp - (uint64_t)cur_thread->user_sp;
    // print_str("\nParent sp_el0: ");
    // print_hex(tpf->sp_el0);
    // print_str("\nChild sp_el0: ");
    // print_hex(child_tpf->sp_el0);

    push_rq(child_thread);

    // check_RQ();

    return child_thread->id;
}

void new_user_thread(void* prog){
    thread_t* new_thread = _thread_create(user_thread_run); 

    // if (prog_size > 0){
    //     new_thread->prog_size = prog_size;
    //     new_thread->prog = malloc(prog_size);
    //     memcpy(prog, new_thread->prog, prog_size);
    // }else {
    //     new_thread->prog = prog;
    // }

    new_thread->ctx.x19 = prog;
    push_rq(new_thread);

    // thread_t* off_thread = cur_thread;
    // off_thread->status = READY;

    // cur_thread = new_thread;
    // cur_thread->status = RUNNING;
    
    // push_rq(off_thread);
    // switch_to(get_current(), &(new_thread->ctx));
}

void user_thread_run(){

    disable_interrupt();

    asm volatile(
        "msr    spsr_el1, xzr;"
        "msr    elr_el1, x19;"
        "msr    sp_el0, %0;"
        ::  
        "r"(cur_thread->user_sp + USTACK_SIZE)
    );

    enable_interrupt();
    asm volatile("eret;");
    
    // thread_exit();
}

void push_rq(thread_t* th){

    if (RQ_tail != 0){

        // async_uart_puts("\nThread ID: ");
        // async_uart_dec(th->id);

        RQ_tail->next = th;
        th->prev = RQ_tail;
        RQ_tail = RQ_tail->next;

        // check_RQ();

    }else {
        RQ_head = th;
        RQ_tail = RQ_head;
    }

}

thread_t* pop_rq(){

    thread_t* target_thread = RQ_head;

    if (RQ_head){
        RQ_head = RQ_head->next;

        if (RQ_head != 0){
            RQ_head->prev = 0;
        }else {
            RQ_tail = 0;
        }
    }

    if (target_thread)
        target_thread->next = 0;

    return target_thread;
}

void check_RQ(){
    thread_t* th_ptr = RQ_head;
    async_uart_newline();

    while (th_ptr){
        async_uart_hex(th_ptr->id);
        async_uart_puts(" -> ");
        th_ptr = th_ptr->next;
    }

    // async_uart_newline();
    // th_ptr = RQ_head;

    // while (th_ptr){
    //     async_uart_hex(th_ptr->status);
    //     async_uart_puts(" -> ");
    //     th_ptr = th_ptr->next;
    // }
}

// For thread test
void foo(){
    for(int i = 0; i < 10; ++i) {
        async_uart_puts("\nThread ID: ");
        async_uart_dec(cur_thread->id);
        async_uart_puts(" ");
        async_uart_dec(i);
        // equivalent to: printf("Thread id: %d %d\n", current_thread().id(), i);
        delay(1000000);
        schedule();
    }

    thread_exit();
}