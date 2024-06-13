#include "kernel/uart.h"
#include "kernel/INT.h"
#include "kernel/gpio.h"
#include "kernel/timer.h"
#include "kernel/task.h"
#include "kernel/syscall.h"
// test_NI is for testing nested interrupt
int test_NI = 0;

//static unsigned long long int_off_count = 0;
void int_off(void){
    asm volatile(
        "msr daifset, 0xf;"
    );
}

void int_on(void){
    asm volatile(
        "msr daifclr, 0xf;"
    );
}
// x0 = tf
int c_system_call_handler(trap_frame_t *tf, my_uint64_t args){
    int_on();
    /*uart_puts("Entering system call handler\n");

    void *spsr1;
    void *elr1;
    void *esr1;
    void *el;

    asm volatile(
        "mrs %[var1], spsr_el1;"
        "mrs %[var2], elr_el1;"
        "mrs %[var3], esr_el1;"
        "mrs %[var4], CurrentEL;"
        : [var1] "=r" (spsr1),[var2] "=r" (elr1),[var3] "=r" (esr1),[var4] "=r" (el)    // Output operands
    );

    uart_puts("Current EL:");
    uart_b2x_64((unsigned long long)el>>2);     // bits [3:2] contain current El value
    uart_putc('\n');

    uart_puts("SPSR_EL1:  ");
    uart_b2x_64((unsigned long long)spsr1);
    uart_putc('\n');

    uart_puts("ELR_EL1:   ");
    uart_b2x_64((unsigned long long)elr1);
    uart_putc('\n');

    uart_puts("ESR_EL1:   ");
    uart_b2x_64((unsigned long long)esr1);
    uart_putc('\n');*/

    // based on the lab instruction. The system call numbers given below would be stored in x8
    unsigned long long syscall_num = tf->x8;
    //uart_b2x_64(syscall_num);
    //uart_putc('\n');
    current_tf = tf;
    //uart_b2x_64((unsigned long long)syscall_num);
    // uart_putc('\n');

    int val = -1;

    switch (syscall_num){
        case 0:
            val = getpid();
            break;
        case 1:
            val = uart_read((char *)current_tf->x0, current_tf->x1);
            break;
        case 2:
            val = uart_write((char *)current_tf->x0, current_tf->x1);
            break;
        case 3:
            uart_b2x_64((unsigned long long)syscall_num);
            uart_putc('\n');
            val = exec((const char *)current_tf->x0, (char *const *)current_tf->x1);
            break;
        case 4:
            uart_b2x_64((unsigned long long)syscall_num);
            uart_putc('\n');
            val = fork(args);
            break;
        case 5:
            uart_b2x_64((unsigned long long)syscall_num);
            uart_putc('\n');
            exit();
            break;
        case 6:
            uart_b2x_64((unsigned long long)syscall_num);
            uart_putc('\n');
            val = mbox_call((unsigned char)current_tf->x0, (unsigned int *)current_tf->x1);
            break;
        case 7:
            uart_b2x_64((unsigned long long)syscall_num);
            uart_putc('\n');
            kill((int)current_tf->x0);
            break;
        case 8:
            uart_b2x_64((unsigned long long)syscall_num);
            uart_putc('\n');
            sigreg((int)current_tf->x0, (void (*)())current_tf->x1);
            break;
        case 9:
            uart_b2x_64((unsigned long long)syscall_num);
            uart_putc('\n');
            sigkill((int)current_tf->x0, (int)current_tf->x1);
            break;
        case 11:
            // uart_b2x_64((unsigned long long)syscall_num);
            // uart_putc('\n');
            val = open((const char *)current_tf->x0, (int)current_tf->x1);
            break;
        case 12:
            // uart_b2x_64((unsigned long long)syscall_num);
            // uart_putc('\n');
            val = close((int)current_tf->x0);
            break;
        case 13:
            // uart_b2x_64((unsigned long long)syscall_num);
            // uart_putc('\n');
            val = write((int)current_tf->x0, (const void *)current_tf->x1, (unsigned long)current_tf->x2);
            break;
        case 14:
            // uart_b2x_64((unsigned long long)syscall_num);
            // uart_putc('\n');
            val = read((int)current_tf->x0, (void *)current_tf->x1, (unsigned long)current_tf->x2);
            break;
        case 15:
            // uart_b2x_64((unsigned long long)syscall_num);
            // uart_putc('\n');
            val = mkdir((const char *)current_tf->x0, (unsigned)current_tf->x1);
            break;
        case 16:
            // uart_b2x_64((unsigned long long)syscall_num);
            // uart_putc('\n');
            val = mount((const char *)current_tf->x0, (const char *)current_tf->x1, (const char *)current_tf->x2, (unsigned long)current_tf->x3, (const void *)current_tf->x4);
            break;
        case 17:
            // uart_b2x_64((unsigned long long)syscall_num);
            // uart_putc('\n');
            val = chdir((const char *)current_tf->x0);
            break;
        case 18:   
            // uart_b2x_64((unsigned long long)syscall_num);
            // uart_putc('\n');
            val = lseek64((int)current_tf->x0, (long)current_tf->x1, (int)current_tf->x2);
            break;
        case 19:
            // uart_b2x_64((unsigned long long)syscall_num);
            // uart_putc('\n');
            val = ioctl((int)current_tf->x0, (unsigned long)current_tf->x1, (void *)current_tf->x2);
            break;
        case 64:
            uart_b2x_64((unsigned long long)syscall_num);
            sigret();
            break;
        default:
            uart_puts("Unknown system call number: ");
            uart_b2x_64(syscall_num);
            uart_putc('\n');
            break;
    }

    return val;

    // while(1)
    //     asm volatile("nop");
}

void c_exception_handler(){
    void *spsr1;
    void *elr1;
    void *esr1;
    void *el;

    uart_puts("Entering exception handler\n");

    asm volatile(
        "mrs %[var1], spsr_el1;"
        "mrs %[var2], elr_el1;"
        "mrs %[var3], esr_el1;"
        "mrs %[var4], CurrentEL;"
        : [var1] "=r" (spsr1),[var2] "=r" (elr1),[var3] "=r" (esr1),[var4] "=r" (el)    // Output operands
    );

    uart_puts("Current EL:");
    uart_b2x_64((unsigned long long)el>>2);     // bits [3:2] contain current El value
    uart_putc('\n');

    uart_puts("SPSR_EL1:  ");
    uart_b2x_64((unsigned long long)spsr1);
    uart_putc('\n');

    uart_puts("ELR_EL1:   ");
    uart_b2x_64((unsigned long long)elr1);
    uart_putc('\n');

    uart_puts("ESR_EL1:   ");
    uart_b2x_64((unsigned long long)esr1);
    uart_putc('\n');

    uart_puts("Leaving exception handler\n");
    
    while(1){
        asm volatile("nop");
    }
}

void c_core_timer_handler(){
    unsigned long long cur_cnt, cnt_freq, el;

    asm volatile(
        "mrs %[var1], cntpct_el0;"
        "mrs %[var2], cntfrq_el0;"
        "mrs %[var3], CurrentEL;"
        :[var1] "=r" (cur_cnt), [var2] "=r" (cnt_freq), [var3] "=r" (el)
    );
    uart_puts("Current EL:");
    uart_b2x_64((unsigned long long)el>>2);     // bits [3:2] contain current El value
    uart_putc('\n');

    uart_puts("Time after boots: ");
    uart_b2x_64(cur_cnt / cnt_freq);
    uart_puts(" sec.\n");

    cnt_freq *= 2;

    /*asm volatile(
        "msr cntp_tval_el0, %[var1];"
        "msr spsr_el1, %[var2];"
        :
        :[var1] "r" (cnt_freq), [var2] "r" (0)
        :
    );*/
    asm volatile(
        "msr cntp_tval_el0, %[var1];"
        :
        :[var1] "r" (cnt_freq)
        :
    );
}

void c_write_handler(){
    //mmio_write((long)AUX_MU_IER_REG, *AUX_MU_IER_REG | 0x2);

    while(write_index_cur != write_index_tail){
        char c = write_buffer[write_index_cur++];

        // If no CR, first line of output will be moved right for n chars(n=shell command just input), not sure why
        if(c == '\n'){
            while(!((*AUX_MU_LSR_REG) & 0x20) ){
                asm volatile("nop");
            }
            *AUX_MU_IO_REG = '\r';
        }

        // check if FIFO can accept at least one byte after sending one character
        while(!((*AUX_MU_LSR_REG) & 0x20) ){
            // if bit 5 is set, break and return IO_REG
            asm volatile("nop");
        }
        *AUX_MU_IO_REG = c;
        
        write_index_cur = write_index_cur % MAX_BUF_LEN;
    }
    // disable writer interrupt
    mmio_write((long)AUX_MU_IER_REG, *AUX_MU_IER_REG | ~(0x2));
}

void c_recv_handler(){

    char c = uart_getc();

    read_buffer[read_index_tail++] = c;
    read_index_tail = read_index_tail % MAX_BUF_LEN;

    // Put into write buffer such that the received char can be echoed out
    //write_buffer[write_index_tail++] = c;
    //write_index_tail = write_index_tail % MAX_BUF_LEN;

    //task_create_DF0(c_write_handler, 0);

    //uart_putc(c);
    
    // Enable receiver interrupt
    mmio_write((long)AUX_MU_IER_REG, *AUX_MU_IER_REG | 0x1);
    //uart_puts("End of recv handler\n");
}
void gdb_brk(){
    uart_puts("this is just for gdb\n");
}
void handler1(){
    // clear register to prevent keeping interrupting
    uart_getc();
    uart_puts("handler1\n");
    // use write interrupt to test nested interrupt
    mmio_write((long)AUX_MU_IER_REG, *AUX_MU_IER_REG | (0x2));
    delay(10000);
    uart_puts("handler1 end\n");
    PRI_TEST_FLAG = 1;
    gdb_brk();
}

void handler2(){
    uart_puts("handler2\n");
    PRI_TEST_FLAG = 1;
}

void c_timer_handler(){
    unsigned long long cur_cnt, value;
    task_timer_t *cur = timer_head;

    if(cur == 0)
        return;

    value = 0;
    // disable interrupt to protect critical section
    asm volatile(
        "msr cntp_ctl_el0, %[var1];"
        :
        :[var1] "r" (value)
    );

    asm volatile(
        "msr daifset, 0xf;"
    );

    asm volatile(
        "mrs %[var1], cntpct_el0;"
        :[var1] "=r" (cur_cnt)
    );
    //uart_b2x_64(cur_cnt);
    //uart_putc('\n');

    while(cur_cnt >= cur->deadline){
        cur->callback(cur->data);
        
        timer_head = cur->next;
        if(timer_head != 0){
            timer_head->prev = 0;
            // If the timeout is earlier than the previous programed expired time, the kernel reprograms the hardware timer to the earlier one.
            // enable timer
            value = 1;
            asm volatile(
                "msr cntp_cval_el0, %[var1];"
                "msr cntp_ctl_el0, %[var2];"
                :
                :[var1] "r" (timer_head->deadline), [var2] "r" (1)
            );
            // Since we turn off the core0 timer interrupt in general interrupt handler, we need to turn it on again
            mmio_write((long)CORE0_TIMER_IRQ_CTRL, 2);
        }
        else{
            uart_puts("No more timer\n");
            // disable timer
            value = 0;
            asm volatile(
                "msr cntp_ctl_el0, %[var1];"
                :
                :[var1] "r" (value)
            );

            break;
        }
        // free cur(not implemented)
        cur = cur->next;
    }

    // enable all interrupt of current EL
    asm volatile(
        "msr daifclr, 0xf;"
    );
}

void c_general_irq_handler(trap_frame_t *tf){
    unsigned int cpu_irq_src, gpu_irq_src;
    unsigned long long el;

    asm volatile(
        "mrs %[var1], CurrentEL;"
        :[var1] "=r" (el)
    );
    
    /*uart_puts("Current EL:");
    uart_b2x_64((unsigned long long)el>>2);     // bits [3:2] contain current El value
    uart_putc('\n');*/

    cpu_irq_src = mmio_read((long)CORE0_INT_SRC);
    
    // Through this, we can see that uart interrupt is in pending register 1(0x00000100)
    /*irq_src = mmio_read((long)IRQ_basic_pending);
    uart_puts("basic pending:");
    uart_b2x(irq_src);
    uart_putc('\n');*/

    gpu_irq_src = mmio_read((long)IRQ_pending_1);
    
    // There periphial interrupt in pending 1 register
    // if bit29, meaning a async write or read
    if(gpu_irq_src & (1 << 29)){
        //uart_puts("Periphial IRQ\n");

        unsigned int irq_status = mmio_read((long)AUX_MU_IIR_REG);
        //uart_b2x(irq_status);
        // [2:1]=10 : Receiver holds valid byte 
        if(irq_status & 0x4){
            // disable receive interrupt by setting bit1 to 0
            //uart_puts("Receive IRQ\n");
            mmio_write((long)AUX_MU_IER_REG, *AUX_MU_IER_REG & ~(0x1));
            if(test_NI == 0)
                task_create_DF0(c_recv_handler, 1);
            else if(test_NI == 1)
                task_create_DF0(handler1, 0);
            else if(test_NI == 2)
                task_create_DF0(handler1, 1);
            prep_task();

            //c_recv_handler();
        }
        // [2:1]=01 : Transmit holding register empty
        if(irq_status & 0x2){
            //uart_puts("Transmit IRQ\n");
            // disable transmit interrupt, set bit2 to 0
            mmio_write((long)AUX_MU_IER_REG, *AUX_MU_IER_REG & ~(0x2));
            //c_write_handler();
            if(test_NI == 0)
                task_create_DF0(c_write_handler, 0);
            else if(test_NI == 1)
                task_create_DF0(handler2, 1);
            else if(test_NI == 2)
                task_create_DF0(handler2, 0);
            prep_task();
        }
    }
    // CNTPNSIRQ interrupt bit, this is by observation, not quite sure why is that bit(which is Non-secure physical timer event.)
    // https://developer.arm.com/documentation/100964/1118/Fast-Models-components/SystemIP-components/GIC-400
    if(cpu_irq_src & (0x1 << 1)){
        //uart_puts("Timer IRQ\n");

        if(boot_timer_flag == 2){
            mmio_write((long)CORE0_TIMER_IRQ_CTRL, 0);
            //task_create_DF0(process_schedule, 0);
            //process_schedule();
            task_create_DF0(c_timer_handler, 0);
            prep_task();
            mmio_write((long)CORE0_TIMER_IRQ_CTRL, 2);
            process_schedule();
        }
        else if(boot_timer_flag != 0){
            c_core_timer_handler();
        }
        else{
            // disable core0 timer interrupt, 
            // p.13 https://github.com/Tekki/raspberrypi-documentation/blob/master/hardware/raspberrypi/bcm2836/QA7_rev3.4.pdf
            mmio_write((long)CORE0_TIMER_IRQ_CTRL, 0);
            task_create_DF0(c_timer_handler, 0);
            prep_task();
        }
    }
    if((tf->spsr_el1 & 0b1100) == 0){
        //uart_puts("EL0\n");
        check_signal(tf);
    }
    // Save the current state
    /*asm volatile(
        "mrs %[var1], spsr_el1;"
        "mrs %[var2], elr_el1;"
        : [var1] "=r" (spsr1), [var2] "=r" (elr1)
    );
    // enable all interrupt
    asm volatile(
        "msr daifclr, 0xf;"
    );
    ExecTasks();
    // Restore the previous state
    asm volatile(
        "msr spsr_el1, %[var1];"
        "msr elr_el1, %[var2];"
        :
        : [var1] "r" (spsr1), [var2] "r" (elr1)
    );*/
}